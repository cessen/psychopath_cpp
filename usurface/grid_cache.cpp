#include "numtype.h"

#include "config.hpp"
#include "grid_cache.hpp"

namespace GridCache
{
LRUCache<Grid> cache(Config::grid_cache_size * (1000*1000));
}
