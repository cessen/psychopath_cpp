#ifndef RNG_HPP
#define RNG_HPP

// UINT32_C is defined here to work around lack of stdint.h defs in C++
// There's probably a more proper way to handle this.
#define UINT32_C(value)   value##u

extern "C" {
#include "tinymt32.h"
}

#include "time.h"

/**
 * @brief A psuedo-random number generator.
 *
 * Based on Tiny Mersenne Twister.
 */
class RNG
{
public:
	tinymt32_t state;

	/**
	 * @brief Constructor.
	 *
	 * Initializes the RNG with a default seed (based on time).
	 * Initializing an RNG this way is not recommended.
	 */
	RNG() {
		tinymt32_init(&state, time(NULL) + clock());
	}

	/**
	 * @brief Constructor.
	 *
	 * Initializes the RNG with the given seed.  This is the
	 * recommended way to initialize an RNG.
	 */
	RNG(uint32_t seed) {
		tinymt32_init(&state, seed);
	}

	/**
	 * @brief Sets the seed of the RNG.
	 */
	void seed(uint32_t seed) {
		tinymt32_init(&state, seed);
	}

	/**
	 * @brief Returns a random unsigned 32-bit integer.
	 */
	uint32_t next_uint() {
		return tinymt32_generate_uint32(&state);
	}

	/**
	 * @brief Returns a random 32-bit float in the interval [0.0, 1.0).
	 */
	float next_float() {
		return tinymt32_generate_float(&state);
	}

	/**
	 * @brief Returns a random 32-bit float in the interval [-0.5, 0.5).
	 */
	float next_float_c() {
		return tinymt32_generate_float(&state) - 0.5f;
	}
};

#endif

