#ifndef GRID_CACHE_HPP
#define GRID_CACHE_HPP

#include "numtype.h"

#include "grid.hpp"
#include "lru_cache.hpp"

typedef LRUKey GridCacheKey;

namespace GridCache
{
extern LRUCache<Grid> cache;
}

#endif
