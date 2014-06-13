#ifndef BIT_STACK_HPP
#define BIT_STACK_HPP

#include "numtype.h"

#include <cassert>
#include <bitset>

/**
 * A bit field for use as a stack of boolean values, with
 * push, pop, and peek operations.
 */
template <int SIZE>
class BitStack
{
	std::bitset<SIZE> bits {0};

public:
	BitStack() {}
	BitStack(uint32_t init): bits {init} {}

	/**
	 * Push a bit onto the top of the stack.
	 */
	void push(bool value) {
		assert(bits[SIZE-1] == 0); // Verify no stack overflow
		bits <<= 1;
		bits[0] = value;
	}

	/**
	 * Push n bits onto the top of the stack.  The input
	 * bits are passed as an integer, with the bit that
	 * will be on top in the least significant digit, and
	 * the rest following in order from there.
	 */
	void push(uint32_t value, int n) {
		assert(n < SIZE && (bits >> (SIZE-n)).count() == 0); // Verify no stack overflow
		assert(n < 32); // Verify staying within input size
		bits <<= n;
		bits |= value & ((1<<n)-1);
	}

	/**
	 * Pop the top bit off the stack.
	 */
	bool pop() {
		const bool b = bits[0];
		bits >>= 1;
		return b;
	}

	/**
	 * Pop the top n bits off the stack.  The bits are returned as
	 * an integer, with the top bit in the least significant digit,
	 * and the rest following in order from there.
	 */
	uint32_t pop(int n) {
		assert(n < SIZE); // Can't pop more than we have
		assert(n < 32); // Can't pop more than the return type can hold
		const uint32_t b = static_cast<uint32_t>(bits) & ((1<<n)-1);
		bits >>= n;
		return b;
	}

	/**
	 * Read the top bit of the stack without popping it.
	 */
	bool peek() const {
		return bits[0];
	}

	/**
	 * Read the top n bits of the stack without popping them.  The bits
	 * are returned as an integer, with the top bit in the least
	 * significant digit, and the rest following in order from there.
	 */
	bool peek(int n) const {
		assert(n < SIZE); // Can't return more than we have
		assert(n < 32); // Can't return more than the return type can hold
		return static_cast<uint32_t>(bits) & ((1<<n)-1);
	}

	/**
	 * Read any bit of the stack, by index.
	 */
	bool operator[](int pos) const {
		assert(pos < SIZE); // Verify access bounds
		return bits[pos];
	}
};

#endif // BIT_STACK_HPP