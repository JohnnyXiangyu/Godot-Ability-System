#include "ability_system_client.h"

char AbilitySystemClient::signal_name_vector_change[] = "asc_tag_changed";
char AbilitySystemClient::signal_name_scaler_change[] = "asc_attribute_changed";
char AbilitySystemClient::signal_name_owning_effect_lifted[] = "asc_effect_lifted";

void AbilitySystemClient::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_tag", "tag"), &AbilitySystemClient::get_vector_attributes);
	ClassDB::bind_method(D_METHOD("get_attribute", "attribute"), &AbilitySystemClient::get_scaler_attributes);
	ClassDB::bind_method(D_METHOD("interop_instigate_raw", "target", "tags", "events", "modifier_operations", "modifier_magnitudes", "modifier_attributes", "duration", "hint_information"), &AbilitySystemClient::instigate_lasting_raw);
	ClassDB::bind_method(D_METHOD("interop_instant_raw", "target", "modifier_operations", "modifier_magnitudes", "modifier_attributes", "hint_information"), &AbilitySystemClient::instigate_instant_raw);
	ClassDB::bind_method(D_METHOD("lift_instigation", "handle"), &AbilitySystemClient::lift_instigation);

	ClassDB::bind_method(D_METHOD("set_tick", "tick"), &AbilitySystemClient::set_tick);
	ClassDB::bind_method(D_METHOD("get_tick"), &AbilitySystemClient::get_tick);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "tick"), "set_tick", "get_tick");

	ADD_SIGNAL(MethodInfo(signal_name_vector_change, PropertyInfo(Variant::INT, "tag_id"), PropertyInfo(Variant::PACKED_VECTOR2_ARRAY, "new_events")));
	ADD_SIGNAL(MethodInfo(signal_name_scaler_change, PropertyInfo(Variant::INT, "attribute_id"), PropertyInfo(Variant::FLOAT, "new_value")));
	ADD_SIGNAL(MethodInfo(signal_name_owning_effect_lifted, PropertyInfo(Variant::RID, "instigation_handle")));
}

void AbilitySystemClient::_notification(int p_what) {
#ifdef TOOLS_ENABLED
	if (is_part_of_edited_scene())
		return;
#endif

	switch (p_what) {
		case NOTIFICATION_PROCESS: {
			float delta = get_process_delta_time();
			duration_list.update(delta);
			RID timedout;
			while (duration_list.try_timeout(timedout)) {
				lift_instigation(timedout);
			}
			break;
		}
	}
}

void AbilitySystemClient::recalculate_attributes() {
	for (KeyValue<int, float> &adds : scalers_add) {
		adds.value = 0;
	}

	for (KeyValue<int, float> &mults : scalers_mult) {
		mults.value = 1;
	}

	for (const RID &handle : received_instigations) {
		InstigationContext *context = instigation_owner.get_or_null(handle);
		for (const ScalerModifier &mod : context->attribute_modifiers) {
			switch (mod.operation)
			{
				case ModifierAction::MODIFIER_ACTION_ADD:
					scalers_add[mod.attribute] = get_attribute_add(mod.attribute) + mod.magnitude;
					break;
				case ModifierAction::MODIFIER_ACTION_MULTIPLY:
					scalers_mult[mod.attribute] = get_attribute_mult(mod.attribute) * mod.magnitude;
			}
		}
	}
}

float AbilitySystemClient::get_attribute_base_value(int p_attribute_id) const {
	HashMap<int, float>::ConstIterator base = scaler_base_values.find(p_attribute_id);
	if (base == scaler_base_values.end())
		return 0;
	return base->value;
}

float AbilitySystemClient::get_attribute_add(int p_attribute_id) const {
	HashMap<int, float>::ConstIterator added = scalers_add.find(p_attribute_id);
	if (added == scalers_add.end())
		return 0;
	return added->value;
}

float AbilitySystemClient::get_attribute_mult(int p_attribute_id) const {
	HashMap<int, float>::ConstIterator multed = scalers_mult.find(p_attribute_id);
	if (multed == scalers_add.end())
		return 1;
	return multed->value;
}

Vector<Vector2> AbilitySystemClient::get_vector_attributes(int p_tag) const {
	return vector_lists[p_tag];
}

float AbilitySystemClient::get_scaler_attributes(int p_attribute_id) const {
	return (get_attribute_base_value(p_attribute_id) + get_attribute_add(p_attribute_id)) * get_attribute_mult(p_attribute_id);
}

RID AbilitySystemClient::apply_effect_core(const InstigationContext &p_context) {
	RID new_handle = instigation_owner.make_rid(p_context);

	// update tags
	for (int i = 0; i < p_context.tag_modifiers.size(); i++) {
		TaggedEvent context_event = p_context.tag_modifiers[i];
		PackedVector2Array &found_events = vector_lists[context_event.tag_id];
		found_events.push_back(context_event.event_data);
		changed_vectors.insert(context_event.tag_id);
	}

	// update attributes
	received_instigations.push_back(new_handle);
	for (int i = 0; i < p_context.attribute_modifiers.size(); i++) {
		const ScalerModifier &attr_mod = p_context.attribute_modifiers[i];
		changed_scalers.insert(attr_mod.attribute);
	}
	recalculate_attributes();

	// notify observers
	signal_value_change();

	// add the context to the owner
	return new_handle;
}

void AbilitySystemClient::remove_instigation_core(RID p_handle) {
	// handles can be invalid
	InstigationContext* confirmed_context = instigation_owner.get_or_null(p_handle);
	if (confirmed_context == NULL)
		return;

	// update tags
	for (const TaggedEvent &context_event : confirmed_context->tag_modifiers) {
		HashMap<int, PackedVector2Array>::Iterator event_container = vector_lists.find(context_event.tag_id);
		if (event_container == vector_lists.end())
			continue;
		event_container->value.erase(context_event.event_data);
		changed_vectors.insert(context_event.tag_id);
	}

	// update attributes
	received_instigations.erase(p_handle);
	for (const ScalerModifier &attr_mod : confirmed_context->attribute_modifiers) {
		changed_scalers.insert(attr_mod.attribute);
	}
	recalculate_attributes();

	// notify observers
	signal_value_change();

	// notify the instiagor
	confirmed_context->instigator->emit_signal(signal_name_owning_effect_lifted, p_handle);

	// free the RID thus invalidate it
	instigation_owner.free(p_handle);
}

void AbilitySystemClient::signal_value_change() {
	// tags
	for (const int &tag : changed_vectors) {
		const PackedVector2Array &events = get_vector_attributes(tag);
		emit_signal(SNAME(signal_name_vector_change), tag, events);
	}
	changed_vectors.clear();

	// attributes
	for (const int &attribute : changed_scalers) {
		float new_attribute = get_scaler_attributes(attribute);
		emit_signal(SNAME(signal_name_scaler_change), attribute, new_attribute);
	}
	changed_scalers.clear();
}

RID AbilitySystemClient::instigate_lasting(AbilitySystemClient *p_target, const Vector<TaggedEvent> &p_tag_changes, const Vector<ScalerModifier> &p_attribute_modifiers, float p_duration) {
	InstigationContext new_context(this, p_target, p_tag_changes, p_attribute_modifiers);
	RID new_effect = p_target->apply_effect_core(new_context);

	if (p_duration > 0) {
		p_target->duration_list.add_duration_effect(p_duration, new_effect);
	}

	return new_effect;
}

RID AbilitySystemClient::instigate_lasting_raw(AbilitySystemClient *p_target, const Vector<int> &p_tags, const Vector<Vector2> &p_events, const Vector<int> &p_modifier_operations, const Vector<float> &p_modifier_magnitudes, const Vector<int> &p_modifier_attributes, float p_duration, String hint_information) {
	if (p_tags.size() != p_events.size() || p_modifier_attributes.size() != p_modifier_magnitudes.size() || p_modifier_magnitudes.size() != p_modifier_operations.size()) {
		print_error(vformat("mismatch in raw data lengths for %s", hint_information));
	}

	Vector<TaggedEvent> tag_events;
	if (p_tags.size() > 0) {
		for (int i = 0; i < p_tags.size(); i++) {
			TaggedEvent new_event;
			new_event.tag_id = p_tags[i];
			new_event.event_data = p_events[i];
			tag_events.push_back(new_event);
		}
	}

	Vector<ScalerModifier> attribute_modifiers;
	if (p_modifier_attributes.size() > 0) {
		for (int i = 0; i < p_modifier_attributes.size(); i++) {
			ScalerModifier new_mod;
			new_mod.attribute = p_modifier_attributes[i];
			new_mod.magnitude = p_modifier_magnitudes[i];
			new_mod.operation = p_modifier_operations[i];
			attribute_modifiers.push_back(new_mod);
		}
	}

	return instigate_lasting(p_target, tag_events, attribute_modifiers, p_duration);
}

void AbilitySystemClient::instigate_instant(AbilitySystemClient *p_target, const Vector<ScalerModifier> &p_attribute_modifiers) {
	// update attribute base values
	for (int i = 0; i < p_attribute_modifiers.size(); i++) {
		const ScalerModifier &attr_mod = p_attribute_modifiers[i];
		changed_scalers.insert(attr_mod.attribute);

		switch (attr_mod.operation) {
			case ModifierAction::MODIFIER_ACTION_ADD:
				scaler_base_values[attr_mod.attribute] = get_attribute_base_value(attr_mod.attribute) + attr_mod.magnitude;
				break;
			case ModifierAction::MODIFIER_ACTION_MULTIPLY:
				scaler_base_values[attr_mod.attribute] = get_attribute_base_value(attr_mod.attribute) * attr_mod.magnitude;
				break;
		}
	}

	// notify observers
	signal_value_change();
}

// reimplement these since there's no need for a concentrated record for individual instigations and skipping the packing process could potentially save a lot of performance
void AbilitySystemClient::instigate_instant_raw(AbilitySystemClient* p_target, const Vector<int>& p_modifier_operations, const Vector<float>& p_modifier_magnitudes, const Vector<int>& p_modifier_attributes, String hint_information) {
	if (p_modifier_attributes.size() != p_modifier_magnitudes.size() || p_modifier_magnitudes.size() != p_modifier_operations.size()) {
		print_error(vformat("mismatch in raw data lengths for %s", hint_information));
	}

	// update attribute base values
	for (int i = 0; i < p_modifier_operations.size(); i++) {
		ScalerModifier new_mod;
		new_mod.attribute = p_modifier_attributes[i];
		new_mod.magnitude = p_modifier_magnitudes[i];
		new_mod.operation = p_modifier_operations[i];

		changed_scalers.insert(new_mod.attribute);

		switch (p_modifier_operations[i]) {
			case ModifierAction::MODIFIER_ACTION_ADD:
				scaler_base_values[new_mod.attribute] = get_attribute_base_value(new_mod.attribute) + new_mod.magnitude;
				break;
			case ModifierAction::MODIFIER_ACTION_MULTIPLY:
				scaler_base_values[new_mod.attribute] = get_attribute_base_value(new_mod.attribute) * new_mod.magnitude;
				break;
		}
	}

	// notify observers
	signal_value_change();
}

bool AbilitySystemClient::lift_instigation(const RID &p_handle) {
	if (!instigation_owner.owns(p_handle))
		return false;

	remove_instigation_core(p_handle);
	return true;
}

void AbilitySystemClient::set_tick(bool p_tick) {
	if (p_tick == tick)
		return;

	set_process(p_tick);
	tick = p_tick;
}

bool AbilitySystemClient::get_tick() const {
	return tick;
}
