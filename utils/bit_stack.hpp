#ifndef BIT_STACK_HPP
#define BIT_STACK_HPP

#include "numtype.h"

#include <cassert>
#include <bitset>

/**
 * A bit field that can be used as a stack of
 * booleans, with push and pop operations.
 */
template <int SIZE>
class BitStack
{
	std::bitset<SIZE> bits {0};

public:
	bool operator[](size_t pos) const {
		return bits[pos];
	}

	void push(bool v) {
		bits <<= 1;
		bits.set(0, v);
	}

	void push(uint32_t value, int num_bits) {
		bits <<= num_bits;
		bits |= std::bitset<SIZE>(value);
	}

	bool pop() {
		const bool b = bits.test(0);
		bits >>= 1;
		return b;
	}
};

#endif // BIT_STACK_HPP