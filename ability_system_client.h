#ifndef ABILITY_SYSTEM_CLIENT_H
#define ABILITY_SYSTEM_CLIENT_H

#include "scene/main/node.h"
#include "core/templates/hash_map.h"
#include "core/templates/vector.h"
#include "core/templates/rid.h"

struct LinkedModifier
{
public:
	RID active_effect_handle;
	int action;
	int attribute;
	float magnitude;
};

class AbilitySystemClient : public Node
{
private:
	int current_rid = 0;

protected:
	HashMap<int, int> tags;
	HashMap<int, float> attributes;
	Vector<RID> active_effect_handles;
	Vector<LinkedModifier> active_attribute_modifiers;
};

#endif
