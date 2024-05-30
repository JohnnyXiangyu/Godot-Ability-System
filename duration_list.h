#ifndef DURATION_LIST_H
#define DURATION_LIST_H

#include "core/templates/vector.h"
#include "core/templates/rid.h"

struct DurationEffect {
public:
	float duration_remain;
	RID effect_handle;

	DurationEffect(float p_duration, const RID& p_handle)
	{
		duration_remain = p_duration;
		effect_handle = p_handle;
	}

	DurationEffect() : duration_remain(0), effect_handle()
	{}
};

// TODO: something's wrong, the list isn't updating correctly with notifications
class DurationList
{
private:
	Vector<DurationEffect> data;

	inline int get_parent(int p_child_pos) {
		return (p_child_pos + 1) / 2 - 1;
	}

	inline int get_left_child(int p_parent_pos) {
		return 2 * p_parent_pos + 1;
	}

	inline int get_right_child(int p_parent_pos) {
		return 2 * p_parent_pos + 2;
	}

	inline void swap(int p_pos_1, int p_pos_2) {
		DurationEffect temp_data = data[p_pos_1];
		data.set(p_pos_1, data[p_pos_2]);
		data.set(p_pos_2, temp_data);
	}

	inline void insert(DurationEffect p_new_data) {
		data.push_back(p_new_data);
		int current_pos = data.size() - 1;
		int parent_pos = get_parent(current_pos);
		while (current_pos > 0 && data[current_pos].duration_remain < data[parent_pos].duration_remain) {
			swap(current_pos, parent_pos);
			current_pos = parent_pos;
			parent_pos = get_parent(current_pos);
		}
	}

	inline int get_smaller_child(int p_parent_pos) {
		int left_pos = get_left_child(p_parent_pos);
		int right_pos = get_right_child(p_parent_pos);

		if (left_pos >= data.size())
			return p_parent_pos;

		if (right_pos >= data.size())
			return left_pos;

		const DurationEffect &left_child = data[left_pos];
		const DurationEffect &right_child = data[right_pos];

		if (left_child.duration_remain < right_child.duration_remain) {
			return left_pos;
		} else {
			return right_pos;
		}
	}

	inline bool pop_root(DurationEffect &p_out_value) {
		if (data.size() <= 0)
			return false;

		p_out_value = data[0];
		if (data.size() == 1) {
			data.resize(0);
			return true;
		}

		swap(0, data.size() - 1);
		data.remove_at(data.size() - 1);

		int current_pos = 0;
		while (true) {
			int smaller_child = get_smaller_child(current_pos);
			if (smaller_child >= data.size() || data[current_pos].duration_remain <= data[smaller_child].duration_remain)
				break;

			swap(current_pos, smaller_child);
			current_pos = smaller_child;
		}
		return true;
	}

	inline bool peek_root(DurationEffect& p_out_value) const
	{
		if (data.size() <= 0)
			return false;

		p_out_value = data[0];
		return true;
	}

public:
	void add_duration_effect(float p_duration, const RID &p_effect_handle);
	void update(float p_delta);
	bool try_timeout(RID &p_out_value);
};

#endif // !DURATION_LIST_H
