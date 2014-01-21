#ifndef GRID_CACHE_HPP
#define GRID_CACHE_HPP

#include "numtype.h"

#include <functional>

#include "micro_surface.hpp"
#include "lru_cache.hpp"


namespace MicroSurfaceCache
{
struct Key {
	uint64_t uid1 = 0; // Primary key
	uint64_t uid2 = 1; // Secondary key, for splitting.  Should be 1 for an unsplit primitive

	Key() {}
	Key(uint64_t a, uint64_t b): uid1 {a}, uid2 {b} {}

	// Needed for unordered_map
	bool operator==(const Key& b) const {
		return (uid1 == b.uid1) && (uid2 == b.uid2);
	}
};

extern LRUCache<Key, MicroSurface> cache;
}



// Needed for unordered_map
namespace std
{
template<>
struct hash<MicroSurfaceCache::Key> {
	size_t operator()(MicroSurfaceCache::Key const& k) const {
		size_t const h1(std::hash<uint64_t>()(k.uid1));
		size_t const h2(std::hash<uint64_t>()(k.uid2));
		return h1 ^ ((h2 << 16) | (h2 >> 16));
	}
};
}

#endif
