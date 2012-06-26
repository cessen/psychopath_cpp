#ifndef RNG_HPP
#define RNG_HPP

#include "time.h"
#include "numtype.h"

/**
 * @brief A psuedo-random number generator.
 *
 * Based on the JKISS generator from the paper
 * "Good Practice in (Pseudo) Random Number
 *  Generation for Bioinformatics Applications"
 * by David Jones.
 *
 * This generator is surprisingly robust for how simple it is, passing all of
 * the Dieharder tests as well as the complete Big Crush test set in TestU01.
 * This robustness is comparable to the Mersenne Twister, excepting for the
 * smaller period (~2^127 compared to MT's enormous ~2^19937 period).
 *
 * This PRNG should be more than sufficient for most purposes.
 */
class RNG
{
	uint32 x, y, z, c;

public:
	/**
	 * @brief Constructor.
	 *
	 * Initializes the RNG with a default seed (based on time).
	 * Initializing an RNG this way is not recommended, especially in
	 * software where multiple RNG's are used.
	 */
	RNG() {
		seed(time(NULL) + clock());
	}

	/**
	 * @brief Constructor.
	 *
	 * Initializes the RNG with the given seed.  This is the
	 * recommended way to initialize an RNG.
	 */
	RNG(uint32 seed_) {
		seed(seed_);
	}

	/**
	 * @brief Sets the seed of the RNG.
	 */
	void seed(uint32 seed_) {
		// Make sure seed is large enough
		seed_ += 42;

		// Multiply the seed by various large primes to get our
		// constituent seed values.
		x = seed_ * 3885701021;
		y = seed_ * 653005939;
		z = seed_ * 1264700623;
		c = seed_ * 37452703;
	}

	/**
	 * @brief Returns a random unsigned 32-bit integer.
	 */
	uint32 next_uint() {
		uint64 t;

		x = 314527869 * x + 1234567;

		y ^= y << 5;
		y ^= y >> 7;
		y ^= y << 22;

		t = 4294584393ULL * z + c;
		c = t >> 32;
		z = t;

		return x + y + z;
	}

	/**
	 * @brief Returns a random 32-bit float in the interval [0.0, 1.0).
	 */
	float32 next_float() {
		// The following assumes an IEEE 32-bit binary floating point format.
		// Alternatively, you could just do "next_uint() / 4294967296.0" which
		// would accomplish the same thing, albeit slower.
		union {
			float32 x;
			uint32 a;
		};
		a = next_uint() >> 9; // Take upper 23 bits
		a |= 0x3F800000; // Make a float from bits
		return x-1.f;
	}

	/**
	 * @brief Returns a random 32-bit float in the interval [-0.5, 0.5).
	 */
	float32 next_float_c() {
		return next_float() - 0.5f;
	}
};

#endif // RNG_HPP

