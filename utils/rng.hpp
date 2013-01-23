#ifndef RNG_HPP
#define RNG_HPP

#include <ctime>
#include <cstdint>

#include <mutex>

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
	uint32_t x, y, z, c;

public:
	/**
	 * @brief Constructor.
	 *
	 * Initializes the RNG with a thread-safe incrementing seed.
	 * TODO: verify thread safeness (probably needs work).
	 */
	RNG() {
		static std::mutex mut;

		mut.lock();
		static uint32_t starter_seed = time(NULL) + clock();
		const uint32_t seed_ = ++starter_seed;
		mut.unlock();

		seed(seed_);
	}

	/**
	 * @brief Constructor.
	 *
	 * Initializes the RNG with the given seed.
	 */
	RNG(uint32_t seed_) {
		seed(seed_);
	}

	/**
	 * @brief Sets the seed of the RNG.
	 */
	void seed(uint32_t seed_) {
		// Make sure seed is large enough
		seed_ += 42;

		// Multiply the seed by various large primes to get our
		// constituent seed values.
		x = seed_ * 3885701021;
		y = seed_ * 653005939;
		z = seed_ * 1264700623;
		c = seed_ * 37452703;

		// Run the RNG a couple of times
		next_uint();
		next_uint();
	}

	/**
	 * @brief Returns a random unsigned 32-bit integer.
	 *
	 * This is the work-horse method of the RNG.  All the other next_XXX()
	 * methods call this.
	 */
	uint32_t next_uint() {
		uint64_t t;

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
	float next_float() {
		// The following assumes an IEEE 32-bit binary floating point format.
		// Alternatively, you could just do "next_uint() / 4294967296.0" which
		// would accomplish the same thing, albeit slower.
		union {
			float x;
			uint32_t a;
		};
		a = next_uint() >> 9; // Take upper 23 bits
		a |= 0x3F800000; // Make a float from bits
		return x-1.f;
	}

	/**
	 * @brief Returns a random 32-bit float in the interval [-0.5, 0.5).
	 */
	float next_float_c() {
		return next_float() - 0.5f;
	}
};

#endif // RNG_HPP

