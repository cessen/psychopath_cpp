#include "numtype.h"

#include "grid_cache.h"

namespace GridCache
{
LRUCache<Grid> cache(30 * (1000*1000));
}
