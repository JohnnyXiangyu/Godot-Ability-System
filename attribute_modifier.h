#ifndef ATTRIBUTE_MODIFIER_H
#define ATTRIBUTE_MODIFIER_H

enum ModifierAction {
	MODIFIER_ACTION_ADD = 0,
	MODIFIER_ACTION_MULTIPLY = 1
};

struct ScalerModifier
{
public:
	int operation = 0;
	float magnitude = 0;
	int attribute;
};

#endif // !ATTRIBUTE_MODIFIER_H
