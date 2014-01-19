#include "numtype.h"

#include <functional>

#include "config.hpp"
#include "micro_surface_cache.hpp"

namespace MicroSurfaceCache
{
LRUCache<Key, MicroSurface> cache(Config::grid_cache_size * (1000*1000));
}
