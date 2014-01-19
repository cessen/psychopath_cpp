#ifndef GRID_CACHE_HPP
#define GRID_CACHE_HPP

#include "numtype.h"

#include "micro_surface.hpp"
#include "lru_cache.hpp"

typedef size_t MicroSurfaceCacheKey;

namespace MicroSurfaceCache
{
extern LRUCache<size_t, MicroSurface> cache;
}

#endif
