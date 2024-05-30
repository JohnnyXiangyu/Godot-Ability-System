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

	ScalerModifier() = default;
	ScalerModifier(int p_op, int p_magnitude, int p_attr) :
			operation(p_op), magnitude(p_magnitude), attribute(p_attr) {}
};

#endif // !ATTRIBUTE_MODIFIER_H
