#include "duration_list.h"

void DurationList::add_duration_effect(float p_duration, const RID &p_effect_handle) {
	insert(DurationEffect(p_duration, p_effect_handle));
}

void DurationList::update(float p_delta) {
	for (DurationEffect &effect : data) {
		effect.duration_remain -= p_delta;
	}
}

bool DurationList::try_timeout(RID &p_out_value) {
	DurationEffect temp_result;
	if (!peek_root(temp_result))
		return false;

	if (temp_result.duration_remain > 0)
		return false;

	p_out_value = temp_result.effect_handle;
	return pop_root(temp_result);
}
