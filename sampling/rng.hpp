#ifndef RNG_HPP
#define RNG_HPP

// UINT32_C is defined here to work around lack of stdint.h defs in C++
// There's probably a more proper way to handle this.
#define UINT32_C(value)   value##u

extern "C" {
#include "tinymt32.h"
}

/* Random number generator, based on Tiny Mersenne Twister. */
class RNG
{
public:
	tinymt32_t state;

	RNG() {
		tinymt32_init(&state, 42);
	}

	RNG(uint32_t seed) {
		tinymt32_init(&state, seed);
	}

	void seed(uint32_t seed) {
		tinymt32_init(&state, seed);
	}

	uint32_t next_int() {
		return tinymt32_generate_uint32(&state);
	}

	float next_float() {
		return tinymt32_generate_float(&state);
	}
};

#endif

