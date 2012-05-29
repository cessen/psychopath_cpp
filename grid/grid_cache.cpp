#include "numtype.h"

#include "grid_cache.hpp"

namespace GridCache
{
LRUCache<Grid> cache(30 * (1000*1000));
}
