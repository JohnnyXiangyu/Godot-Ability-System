#include "ability_system_client.h"

void AbilitySystemClient::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_tag", "tag"), &AbilitySystemClient::get_tagged_events);
	ClassDB::bind_method(D_METHOD("get_attribute", "attribute"), &AbilitySystemClient::get_attribute_magnitude);
	ClassDB::bind_method(D_METHOD("interop_instigate_raw", "target", "tags", "events", "modifier_operations", "modifier_magnitudes", "modifier_attributes", "hint_information"), &AbilitySystemClient::instigate_infinite_raw);
	ClassDB::bind_method(D_METHOD("lift_instigation", "handle"), &AbilitySystemClient::lift_instigation);

	ADD_SIGNAL(MethodInfo("asc_tag_changed", PropertyInfo(Variant::INT, "tag_id"), PropertyInfo(Variant::PACKED_VECTOR2_ARRAY, "new_events")));
	ADD_SIGNAL(MethodInfo("asc_attribute_changed", PropertyInfo(Variant::INT, "attribute_id"), PropertyInfo(Variant::FLOAT, "new_value")));
}

void AbilitySystemClient::recalculate_attributes() {
	for (KeyValue<int, float> &adds : attributes_add) {
		adds.value = 0;
	}

	for (KeyValue<int, float> &mults : attributes_mult) {
		mults.value = 1;
	}

	for (const RID &handle : received_instigations) {
		InstigationContext *context = instigation_owner.get_or_null(handle);
		for (const AttributeModifier &mod : context->attribute_modifiers) {
			switch (mod.operation)
			{
				case ModifierAction::MODIFIER_ACTION_ADD:
					attributes_add[mod.attribute] = get_attribute_add(mod.attribute) + mod.magnitude;
					break;
				case ModifierAction::MODIFIER_ACTION_MULTIPLY:
					attributes_mult[mod.attribute] = get_attribute_mult(mod.attribute) * mod.magnitude;
			}
		}
	}
}

float AbilitySystemClient::get_attribute_add(int p_attribute_id) const {
	HashMap<int, float>::ConstIterator added = attributes_add.find(p_attribute_id);
	if (added == attributes_add.end())
		return 0;
	return added->value;
}

float AbilitySystemClient::get_attribute_mult(int p_attribute_id) const {
	HashMap<int, float>::ConstIterator multed = attributes_mult.find(p_attribute_id);
	if (multed == attributes_add.end())
		return 1;
	return multed->value;
}

Vector<Vector2> AbilitySystemClient::get_tagged_events(int p_tag) const {
	return tagged_events[p_tag];
}

float AbilitySystemClient::get_attribute_magnitude(int p_attribute_id) const {
	return get_attribute_add(p_attribute_id) * get_attribute_mult(p_attribute_id);
}

RID AbilitySystemClient::apply_effect_core(const InstigationContext &p_context) {
	RID new_handle = instigation_owner.make_rid(p_context);

	// update tags
	for (int i = 0; i < p_context.tag_modifiers.size(); i++) {
		TaggedEvent context_event = p_context.tag_modifiers[i];
		PackedVector2Array &found_events = tagged_events[context_event.tag_id];
		found_events.push_back(context_event.event_data);
		changed_tags.insert(context_event.tag_id);
	}

	// update attributes
	received_instigations.push_back(new_handle);
	for (int i = 0; i < p_context.attribute_modifiers.size(); i++) {
		const AttributeModifier &attr_mod = p_context.attribute_modifiers[i];
		changed_attributes.insert(attr_mod.attribute);
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
		HashMap<int, PackedVector2Array>::Iterator event_container = tagged_events.find(context_event.tag_id);
		if (event_container == tagged_events.end())
			continue;
		event_container->value.erase(context_event.event_data);
		changed_tags.insert(context_event.tag_id);
	}

	// update attributes
	received_instigations.erase(p_handle);
	for (const AttributeModifier &attr_mod : confirmed_context->attribute_modifiers) {
		changed_attributes.insert(attr_mod.attribute);
	}
	recalculate_attributes();

	// notify observers
	signal_value_change();

	// free the RID thus invalidate it
	instigation_owner.free(p_handle);
}

void AbilitySystemClient::signal_value_change() {
	// tags
	for (const int &tag : changed_tags) {
		const PackedVector2Array &events = get_tagged_events(tag);
		emit_signal(SNAME("asc_tag_changed"), tag, events);
	}
	changed_tags.clear();

	// attributes
	for (const int &attribute : changed_attributes) {
		float new_attribute = get_attribute_magnitude(attribute);
		emit_signal(SNAME("asc_attribute_changed"), attribute, new_attribute);
	}
	changed_attributes.clear();
}

RID AbilitySystemClient::instigate_infinite(AbilitySystemClient *p_target, const Vector<TaggedEvent> &p_tag_changes, const Vector<AttributeModifier> &p_attribute_modifiers) {
	InstigationContext new_context(this, p_target, p_tag_changes, p_attribute_modifiers);
	return p_target->apply_effect_core(new_context);
}

RID AbilitySystemClient::instigate_infinite_raw(AbilitySystemClient *p_target, const Vector<int> &p_tags, const Vector<Vector2> &p_events, const Vector<int> &p_modifier_operations, const Vector<float> &p_modifier_magnitudes, const Vector<int> &p_modifier_attributes, String hint_information) {
	if (p_tags.size() != p_events.size() || p_modifier_attributes.size() != p_modifier_magnitudes.size() || p_modifier_magnitudes.size() != p_modifier_operations.size()) {
		print_error(vformat("mismatch in raw data lengths for %s", hint_information));
	}

	Vector<TaggedEvent> tag_events;
	if (p_tags.size() > 0) {
		tag_events.resize(p_tags.size());
		for (int i = 0; i < p_tags.size(); i++) {
			TaggedEvent new_event;
			new_event.tag_id = p_tags[i];
			new_event.event_data = p_events[i];
			tag_events.push_back(new_event);
		}
	}

	Vector<AttributeModifier> attribute_modifiers;
	if (p_modifier_attributes.size() > 0) {
		attribute_modifiers.resize(p_modifier_attributes.size());
		for (int i = 0; i < p_modifier_attributes.size(); i++) {
			AttributeModifier new_mod;
			new_mod.attribute = p_modifier_attributes[i];
			new_mod.magnitude = p_modifier_magnitudes[i];
			new_mod.operation = p_modifier_operations[i];
			attribute_modifiers.push_back(new_mod);
		}
	}

	return instigate_infinite(p_target, tag_events, attribute_modifiers);
}

bool AbilitySystemClient::lift_instigation(RID p_handle) {
	if (!instigation_owner.owns(p_handle))
		return false;

	remove_instigation_core(p_handle);
	return true;
}
