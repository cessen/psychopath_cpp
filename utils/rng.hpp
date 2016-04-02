#ifndef RNG_HPP
#define RNG_HPP

#include <ctime>
#include <cstdint>

#include <random>
#include <chrono>
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
class RNG {
private:
	uint32_t x, y, z, c;

	/**
	 * @brief Core algorithm of the RNG.
	 *
	 * Progresses an RNG with state variables x_, y_, z_, c_.
	 *
	 * @return The next unsigned 32-bit integer in the random sequence.
	 */
	uint32_t n(uint32_t &x_, uint32_t &y_, uint32_t &z_, uint32_t &c_) {
		uint64_t t;

		x_ = 314527869 * x_ + 1234567;

		y_ ^= y_ << 5;
		y_ ^= y_ >> 7;
		y_ ^= y_ << 22;

		t = 4294584393ULL * z_ + c_;
		c_ = t >> 32;
		z_ = t;

		return x_ + y_ + z_;
	}

public:
	/**
	 * @brief Constructor.
	 *
	 * Initializes the RNG with a thread-safe unique random seed.
	 * Code that uses this constructor can depend on all RNG's from it
	 * being independant with a high level of confidence.
	 */
	RNG() {
		// The seeder is seeded with a combination of random_device,
		// large primes, and the current time.  The idea is that if
		// random_device doesn't function well, the time and the
		// primes function as an okay fall-back.  But ideally
		// random_device functions well.
		std::random_device rd;
		static uint32_t seeder_x = rd() + 2123403127 + std::chrono::high_resolution_clock::now().time_since_epoch().count();
		static uint32_t seeder_y = rd() + 1987607653 + std::chrono::high_resolution_clock::now().time_since_epoch().count();
		static uint32_t seeder_z = rd() + 3569508323 + std::chrono::high_resolution_clock::now().time_since_epoch().count();
		static uint32_t seeder_c = rd() + 5206151 + std::chrono::high_resolution_clock::now().time_since_epoch().count();

		// Use the seeder to create subsequent RNG's that are
		// unique from each other.
		static std::mutex mut;
		mut.lock();
		seed(n(seeder_x, seeder_y, seeder_z, seeder_c),
		     n(seeder_x, seeder_y, seeder_z, seeder_c),
		     n(seeder_x, seeder_y, seeder_z, seeder_c),
		     n(seeder_x, seeder_y, seeder_z, seeder_c));
		mut.unlock();
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
	 * @brief Constructor.
	 *
	 * Initializes the RNG with the given seed.  32-bit variant.
	 */
	RNG(uint32_t seed_) {
		seed(seed_);
	}

	/**
	 * @brief Sets the seed of the RNG.
	 *
	 * Full 128-bit variant.
	 */
	void seed(uint32_t seed_a, uint32_t seed_b, uint32_t seed_c, uint32_t seed_d) {
		x = seed_a;
		y = seed_b;
		z = seed_c;
		c = seed_d;
	}

	/**
	 * @brief Sets the seed of the RNG.
	 *
	 * 32-bit variant, for convenience.
	 */
	void seed(uint32_t seed_) {
		// Scramble up the seed with offsets and multiplications
		// by large primes.
		seed((seed_+ 5) * 3885701021,
		     (seed_ + 43) * 653005939,
		     (seed_ + 13) * 1264700623,
		     (seed_ + 67) * 37452703);

		// Run the RNG a couple of times
		n(x, y, z, c);
		n(x, y, z, c);
	}

	/**
	 * @brief Returns a random unsigned 32-bit integer.
	 */
	uint32_t next_uint() {
		return n(x, y, z, c);
	}

	/**
	 * @brief Returns a random 32-bit float in the interval [0.0, 1.0).
	 */
	float next_float() {
		// The following assumes an IEEE 32-bit binary floating point format.
		// Alternatively, you could just do "next_uint() / 4294967296.0" which
		// would accomplish the same thing, albeit slower.
		union {
			float w;
			uint32_t a;
		};
		a = n(x, y, z, c) >> 9; // Take upper 23 bits
		a |= 0x3F800000; // Make a float from bits
		return w-1.f;
	}

	/**
	 * @brief Returns a random 32-bit float in the interval [-0.5, 0.5).
	 */
	float next_float_c() {
		return next_float() - 0.5f;
	}
};

#endif // RNG_HPP

