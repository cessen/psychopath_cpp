#ifndef INSTANCE_ID_HPP
#define INSTANCE_ID_HPP

#include "numtype.h"
#include <cassert>

static constexpr int MAX_ID_BITS = 64;

struct InstanceID {
	uint64_t id;
	int pos = 0;

	void clear() {
		id = 0;
		pos = 0;
	}

	void push_back(uint64_t sub_id, int bit_length) {
		assert((pos + bit_length) <= MAX_ID_BITS);
		id <<= bit_length;
		id |= sub_id & ((1<<bit_length)-1);
		pos += bit_length;
	}

	uint64_t pop_back(int bit_length) {
		assert((pos - bit_length) >= 0);
		const uint64_t value = id & ((1<<bit_length)-1);
		id >>= bit_length;
		pos -= bit_length;
		return value;
	}

	uint64_t pop_front(int bit_length) {
		assert((pos - bit_length) >= 0);
		const int offset = pos - bit_length;
		const uint64_t value = (id & (((1<<bit_length)-1) << (offset))) >> offset;
		pos -= bit_length;
		return value;
	}
};

#endif // INSTANCE_ID_HPP