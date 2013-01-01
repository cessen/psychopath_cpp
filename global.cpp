#include "global.hpp"

#include "numtype.h"

namespace Global
{
namespace Stats
{
uint64 split_count = 0;
uint64 microsurface_count = 0;
uint64 microelement_count = 0;
uint64 microelement_min_count = 99999999999999;
uint64 microelement_max_count = 0;
uint64 cache_misses = 0;
uint_i primitive_ray_tests = 0;
} // Stats
} // Global
