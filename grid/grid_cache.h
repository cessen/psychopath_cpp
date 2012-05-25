#ifndef GRID_CACHE_H
#define GRID_CACHE_H

#include "numtype.h"

#include "grid.hpp"
#include "lru_cache.h"

typedef LRUKey GridCacheKey;

namespace GridCache
{
    extern LRUCache<Grid> cache;
}

#endif
