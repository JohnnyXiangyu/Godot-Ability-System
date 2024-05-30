#ifndef ABILITY_SYSTEM_CLIENT_H
#define ABILITY_SYSTEM_CLIENT_H

#include "instigation_context.h"
#include "gameplay_tag.h"
#include "attribute_modifier.h"
#include "duration_list.h"

#include "scene/main/node.h"
#include "core/templates/hash_map.h"
#include "core/templates/hash_set.h"
#include "core/templates/rid.h"
#include "core/templates/rid_owner.h"
#include "core/templates/list.h"

class AbilitySystemClient : public Node
{
	GDCLASS(AbilitySystemClient, Node)

private:
	// session state
	RID_Owner<InstigationContext> instigation_owner;
	List<RID> received_instigations;
	DurationList duration_list;

	// TODO: there seems to be an issue with hashsets and to be honest all the collection data structures; getting an iterator out of them seems to be pretty difficult to be consistent
	// TODO: use this new field to set up a test with empty and fresh values to see what happens when I iterate through them
	HashSet<int> raw_value;

	// using Vector to store this frequently mutating value because we don't expect there to be a lot of vector2's stacked at the same time
	HashSet<int> changed_vectors;
	HashMap<int, Vector<Vector2>> vector_lists;

	// attributes are built with 1) base value 2) add value 3) multiply value
	HashSet<int> changed_scalers;
	HashMap<int, float> scaler_base_values;
	HashMap<int, float> scalers_add;
	HashMap<int, float> scalers_mult;

	// used every time attributes change
	void recalculate_attributes();

	// internal getters
	float get_attribute_base_value(int p_attribute_id) const;
	float get_attribute_add(int p_attribute_id) const;
	float get_attribute_mult(int p_attribute_id) const;

	// internal effect management
	RID apply_effect_core(const InstigationContext &p_context);
	void remove_instigation_core(RID p_handle);
	void signal_value_change();

	bool tick = false;

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	// use statically assigned values for signal names and such to reduce chances of introducing human error
	static char signal_name_vector_change[];
	static char signal_name_scaler_change[];
	static char signal_name_owning_effect_lifted[];

	// vector attributes are vect
	Vector<Vector2> get_vector_attributes(int p_tag) const;

	// get the current value of an attribute
	float get_scaler_attributes(int p_attribute_id) const;

	// inflict an effect on the target until you manually lift it
	RID instigate_lasting(AbilitySystemClient *p_target, const Vector<TaggedEvent> &p_vector_changes, const Vector<ScalerModifier> &p_scaler_changes, float p_duration);
	RID instigate_lasting_raw(AbilitySystemClient *p_target, const Vector<int> &p_tags, const Vector<Vector2> &p_vectors, const Vector<int> &p_scaler_operations, const Vector<float> &p_scaler_magnitudes, const Vector<int> &p_scaler_attributes, float p_duration, String hint_information);

	// inflict an effect that will take effect immediately and have no lasting effect
	void instigate_instant(AbilitySystemClient *p_target, const Vector<ScalerModifier> &p_scaler_changes);
	void instigate_instant_raw(AbilitySystemClient *p_target, const Vector<int> &p_scaler_operations, const Vector<float> &p_scaler_magnitudes, const Vector<int> &p_scaler_attributes, String hint_information);

	// remove an existing instigation, returns false if that handle is removed already; this instigation can be either infinite or duration
	bool lift_instigation(const RID &p_handle);

	// used for internal tick
	void set_tick(bool p_tick);
	bool get_tick() const;

	AbilitySystemClient() {}
};

#endif
