#include "global.hpp"

#include <atomic>
#include "numtype.h"

namespace Global
{
namespace Stats
{
std::atomic<uint64> split_count(0);
std::atomic<uint64> microsurface_count(0);
std::atomic<uint64> microelement_count(0);
std::atomic<uint64> microelement_min_count(99999999999999);
std::atomic<uint64> microelement_max_count(0);
std::atomic<uint64> cache_misses(0);
std::atomic<uint_i> primitive_ray_tests(0);
} // Stats
} // Global
