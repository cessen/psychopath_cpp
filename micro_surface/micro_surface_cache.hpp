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
struct std::hash<MicroSurfaceCache::Key> {
	typedef MicroSurfaceCache::Key argument_type;
	typedef std::size_t value_type;

	value_type operator()(argument_type const& k) const {
		value_type const h1(std::hash<uint64_t>()(k.uid1));
		value_type const h2(std::hash<uint64_t>()(k.uid2));
		return h1 ^ (h2 << 16) | (h2 >> 16);
	}
};
}

#endif
