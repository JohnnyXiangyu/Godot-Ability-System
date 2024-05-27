#ifndef ABILITY_SYSTEM_CLIENT_H
#define ABILITY_SYSTEM_CLIENT_H

#include "instigation_context.h"
#include "gameplay_tag.h"
#include "attribute_modifier.h"

#include "scene/main/node.h"
#include "core/templates/hash_map.h"
#include "core/templates/hash_set.h"
#include "core/templates/rid.h"
#include "core/templates/rid_owner.h"
#include "core/object/ref_counted.h"
#include "core/templates/list.h"

class AbilitySystemClient : public Node
{
	GDCLASS(AbilitySystemClient, Node)

private:
	// session state
	RID_Owner<InstigationContext> instigation_owner;
	List<RID> received_instigations;

	// using Vector to store this frequently mutating value because we don't expect there to be a lot of vector2's stacked at the same time
	HashSet<int> changed_tags;
	HashMap<int, Vector<Vector2>> tagged_events;

	// attributes are built out of these two lists
	HashSet<int> changed_attributes;
	HashMap<int, float> attributes_add;
	HashMap<int, float> attributes_mult;

	// used every time attributes change
	void recalculate_attributes();

	// internal getters
	float get_attribute_add(int p_attribute_id) const;
	float get_attribute_mult(int p_attribute_id) const;

	// internal effect management
	RID apply_effect_core(const InstigationContext &p_context);
	void remove_instigation_core(RID p_handle);
	void signal_value_change();

protected:
	static void _bind_methods();

public:
	// get the current count of a tag (0 = none)
	Vector<Vector2> get_tagged_events(int p_tag) const;

	// get the current value of an attribute (
	float get_attribute_magnitude(int p_attribute_id) const;

	// inflict an effect on the target until you manually lift it
	RID instigate_infinite(AbilitySystemClient *p_target, const Vector<TaggedEvent> &p_tag_changes, const Vector<AttributeModifier> &p_attribute_modifiers);
	RID instigate_infinite_raw(AbilitySystemClient *p_target, const Vector<int> &p_tags, const Vector<Vector2> &p_events, const Vector<int> &p_modifier_operations, const Vector<float> &p_modifier_magnitudes, const Vector<int> &p_modifier_attributes, String hint_information);

	// remove an existing instigation, returns false if that handle is removed already; this instigation can be either infinite or duration
	bool lift_instigation(RID p_handle);
};

#endif
