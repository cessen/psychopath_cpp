#ifndef GRID_CACHE_HPP
#define GRID_CACHE_HPP

#include "numtype.h"

#include "micro_surface.hpp"
#include "lru_cache.hpp"

typedef LRUKey MicroSurfaceCacheKey;

namespace MicroSurfaceCache
{
extern LRUCache<MicroSurface> cache;
}

#endif
