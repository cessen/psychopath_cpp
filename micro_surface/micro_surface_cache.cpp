#include "numtype.h"

#include "config.hpp"
#include "micro_surface_cache.hpp"

namespace MicroSurfaceCache
{
LRUCache<size_t, MicroSurface> cache(Config::grid_cache_size * (1000*1000));
}
