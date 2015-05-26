#ifndef BIT_STACK_HPP
#define BIT_STACK_HPP

#include <cassert>

/**
 * A bit field for use as a stack of boolean values, with
 * push, pop, and peek operations.
 */
template <typename INT_TYPE>
class BitStack
{
	enum {
	    NUM_BITS = sizeof(INT_TYPE) * 8
	};

	INT_TYPE bits;

public:
	BitStack() {}
	BitStack(INT_TYPE i): bits {i} {}

	/**
	 * Push a bit onto the top of the stack.
	 */
	void push(bool value) {
		assert(bits >> (NUM_BITS-1) == 0); // Verify no stack overflow
		bits <<= 1;
		bits |= static_cast<uint32_t>(value);
	}

	/**
	 * Push n bits onto the top of the stack.  The input
	 * bits are passed as an integer, with the bit that
	 * will be on top in the least significant digit, and
	 * the rest following in order from there.
	 */
	void push(uint32_t value, int n) {
		assert(n < NUM_BITS && (bits >> (NUM_BITS-n)) ==  0); // Verify no stack overflow
		assert(n < 32); // Verify staying within input size
		bits <<= n;
		bits |= value & ((1<<n)-1);
	}

	/**
	 * Pop the top bit off the stack.
	 */
	bool pop() {
		const bool b = bits & 1;
		bits >>= 1;
		return b;
	}

	/**
	 * Pop the top n bits off the stack.  The bits are returned as
	 * an integer, with the top bit in the least significant digit,
	 * and the rest following in order from there.
	 */
	uint32_t pop(int n) {
		assert(n < NUM_BITS); // Can't pop more than we have
		assert(n < 32); // Can't pop more than the return type can hold
		const uint32_t b = static_cast<uint32_t>(bits) & ((1<<n)-1);
		bits >>= n;
		return b;
	}

	/**
	 * Read the top bit of the stack without popping it.
	 */
	bool peek() const {
		return bits & 1;
	}

	/**
	 * Read the top n bits of the stack without popping them.  The bits
	 * are returned as an integer, with the top bit in the least
	 * significant digit, and the rest following in order from there.
	 */
	bool peek(int n) const {
		assert(n < NUM_BITS); // Can't return more than we have
		assert(n < 32); // Can't return more than the return type can hold
		return static_cast<uint32_t>(bits) & ((1<<n)-1);
	}

	/**
	 * Read any bit of the stack, by index.
	 */
	bool operator[](int pos) const {
		assert(pos < NUM_BITS); // Verify access bounds
		return (bits >> pos) & 1;
	}
};


/**
 * A bit field for use as a stack of boolean values, with
 * push, pop, and peek operations.  Uses two integer types
 * to give twice the stack size at BitStack.
 */
template <typename INT_TYPE>
class BitStack2
{
	enum {
	    INT_SIZE = sizeof(INT_TYPE) * 8,
	    NUM_BITS = sizeof(INT_TYPE) * 16
	};

	INT_TYPE bits1, bits2;

public:
	BitStack2() {}
	BitStack2(INT_TYPE i): bits1 {i} {}

	/**
	 * Push a bit onto the top of the stack.
	 */
	void push(bool value) {
		assert(bits2 >> (INT_SIZE-1) == 0); // Verify no stack overflow
		bits2 = (bits2 << 1) | (bits1 >> (INT_SIZE-1));
		bits1 <<= 1;
		bits1 |= static_cast<uint32_t>(value);
	}

	/**
	 * Push n bits onto the top of the stack.  The input
	 * bits are passed as an integer, with the bit that
	 * will be on top in the least significant digit, and
	 * the rest following in order from there.
	 */
	void push(uint32_t value, int n) {
		assert(n < NUM_BITS && (bits2 >> (INT_SIZE-n)) ==  0); // Verify no stack overflow
		assert(n < 32); // Verify staying within input size
		bits2 = (bits2 << n) | (bits1 >> (INT_SIZE-n));
		bits1 <<= n;
		bits1 |= value & ((1<<n)-1);
	}

	/**
	 * Pop the top bit off the stack.
	 */
	bool pop() {
		const bool b = bits1 & 1;
		bits1 = (bits1 >> 1) | (bits2 << (INT_SIZE-1));
		bits2 >>= 1;
		return b;
	}

	/**
	 * Pop the top n bits off the stack.  The bits are returned as
	 * an integer, with the top bit in the least significant digit,
	 * and the rest following in order from there.
	 */
	uint32_t pop(int n) {
		assert(n < NUM_BITS); // Can't pop more than we have
		assert(n < 32); // Can't pop more than the return type can hold
		const uint32_t b = static_cast<uint32_t>(bits1) & ((1<<n)-1);
		bits1 = (bits1 >> n) | (bits2 << (INT_SIZE-n));
		bits2 >>= n;
		return b;
	}

	/**
	 * Read the top bit of the stack without popping it.
	 */
	bool peek() const {
		return bits1 & 1;
	}

	/**
	 * Read the top n bits of the stack without popping them.  The bits
	 * are returned as an integer, with the top bit in the least
	 * significant digit, and the rest following in order from there.
	 */
	bool peek(int n) const {
		assert(n < NUM_BITS); // Can't return more than we have
		assert(n < 32); // Can't return more than the return type can hold
		return static_cast<uint32_t>(bits1) & ((1<<n)-1);
	}

	/**
	 * Read any bit of the stack, by index.
	 */
	bool operator[](int pos) const {
		assert(pos < NUM_BITS); // Verify access bounds
		return (bits1 >> pos) & 1;
	}
};




#endif // BIT_STACK_HPP