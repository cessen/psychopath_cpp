#ifndef TIMEBOX_HPP
#define TIMEBOX_HPP

#include "numtype.h"

#include <iostream>
#include <stdlib.h>
#include <vector>

template <class T>
class TimeBox
{
public:
	std::vector<T> states;

	TimeBox() {};
	TimeBox(uint8_t state_count): states(state_count) {}
	~TimeBox() {};

	// Initializes the timebox with the given number of states
	void init(uint8_t state_count) {
		states.resize(state_count);
	}

	// Given a time in range [0.0, 1.0], fills in the state indices on
	// either side along with an alpha to blend between them.
	// Returns true on success, false on failure.  Failure typically
	// means that there is only one state in the TimeBox.
	bool query_time(const float &time, int32_t *ia, int32_t *ib, float *alpha) const {
		if (states.size() < 2)
			return false;

		if (time < 1.0) {
			const float temp = time * (states.size() - 1);
			const int32_t index = temp;
			*ia = index;
			*ib = index + 1;
			*alpha = temp - (float)(index);
		} else {
			*ia = states.size() - 2;
			*ib = states.size() - 1;
			*alpha = 1.0;
		}

		return true;
	}

	// Allows transparent access to the underlying state data
	T &operator[](const int32_t &i) {
		return states[i];
	}

	const T &operator[](const int32_t &i) const {
		return states[i];
	}

	TimeBox &operator=(const TimeBox& tb) {
		states = tb.states;
		return *this;
	}

	size_t size() const {
		return states.size();
	}
};







#endif
