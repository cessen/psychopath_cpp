#ifndef HASH_HPP
#define HASH_HPP

#include <cstdint>

static inline uint32_t hash_u32(uint32_t n, uint32_t seed)
{
	uint32_t hash = n;

	for (uint32_t i=0; i < 3; ++i) {
		hash *= 1936502639;
		hash ^= hash >> 16;
		hash += seed;
	}

	return hash;
}

static inline float hash_f32(uint32_t n, uint32_t seed)
{
	uint32_t hash = hash_u32(n, seed);

	union {
		float w;
		uint32_t a;
	};
	a = hash >> 9; // Take upper 23 bits
	a |= 0x3F800000; // Make a float from bits
	return w-1.f;
}

/**
 * @brief A seedable hash class.
 *
 * Takes 32 bit unsigned ints as input, and can output either
 * unsigned 32 bit ints or floats.
 */
class Hash
{
private:
	uint32_t seed {42};

public:
	Hash(uint32_t seed): seed {seed} {}

	/**
	 * @brief Takes an int and returns an int.
	 */
	uint32_t get_int(uint32_t n) {
		return hash_u32(n, seed);
	}

	/**
	 * @brief Takes an int and returns a float in [0, 1).
	 */
	float get_float(uint32_t n) {
		uint32_t hash = hash_u32(n, seed);

		union {
			float w;
			uint32_t a;
		};
		a = hash >> 9; // Take upper 23 bits
		a |= 0x3F800000; // Make a float from bits
		return w-1.f;
	}
};

#endif // HASH_HPP