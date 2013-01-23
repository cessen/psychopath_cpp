#ifndef RNG_HPP
#define RNG_HPP

#include <ctime>
#include <cstdint>

#include <random>
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
	 * Initializes the RNG with a thread-safe unique random seed.
	 * Code that uses this constructor can depend on all RNG's from it
	 * being independant with a high level of confidence.
	 */
	RNG() {
		std::random_device rd;
		
		static std::mutex mut;
		
		// First RNG is seeded with random_device
		static uint32_t next_seed_a = rd();
		static uint32_t next_seed_b = rd();
		static uint32_t next_seed_c = rd();
		static uint32_t next_seed_d = rd();
		
		// Subsequent RNG's are seeded by incrementing the seeds by
		// various primes.
		mut.lock();
		seed(next_seed_a, next_seed_b, next_seed_c, next_seed_d);
		next_seed_a += 3;
		next_seed_b += 5;
		next_seed_c += 7;
		next_seed_d += 11;
		mut.unlock();
	}

	/**
	 * @brief Constructor.
	 *
	 * Initializes the RNG with the given seed.  32-bit variant.
	 */
	RNG(uint32_t seed_) {
		seed(seed_);
	}
	
	/**
	 * @brief Constructor.
	 *
	 * Initializes the RNG with the given seed.  Full 128-bit variant.
	 */
	RNG(uint32_t seed_a, uint32_t seed_b, uint32_t seed_c, uint32_t seed_d) {
		seed(seed_a, seed_b, seed_c, seed_d);
	}

	/**
	 * @brief Sets the seed of the RNG.
	 *
	 * 32-bit variant, for convenience.
	 */
	void seed(uint32_t seed_) {
		seed(seed_, seed_, seed_, seed_);
	}
	
	/**
	 * @brief Sets the seed of the RNG.
	 *
	 * Full 128-bit variant.
	 */
	void seed(uint32_t seed_a, uint32_t seed_b, uint32_t seed_c, uint32_t seed_d) {
		// Scramble up the seeds with offsets and multiplications
		// by large primes.
		x = (seed_a + 5) * 3885701021;
		y = (seed_b + 43) * 653005939;
		z = (seed_c + 13) * 1264700623;
		c = (seed_d + 67) * 37452703;
		
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

