#ifndef INSTIGATION_CONTEXT_H
#define INSTIGATION_CONTEXT_H

#include "attribute_modifier.h"
#include "gameplay_tag.h"
#include "core/templates/vector.h"

class AbilitySystemClient;

struct InstigationContext
{
public:
	Vector<TaggedEvent> tag_modifiers;
	Vector<AttributeModifier> attribute_modifiers;

	AbilitySystemClient *instigator;
	AbilitySystemClient *target;

	// I hate how long this function signature is
	InstigationContext(
		AbilitySystemClient* p_instigator,
		AbilitySystemClient* p_target,
		const Vector<TaggedEvent> &p_tag_modifiers,
		const Vector<AttributeModifier> &p_attribute_modifiers)
	{
		instigator = p_instigator;
		target = p_target;

		tag_modifiers = p_tag_modifiers;
		attribute_modifiers = p_attribute_modifiers;
	}
};

#endif // !INSTIGATION_CONTEXT_H
