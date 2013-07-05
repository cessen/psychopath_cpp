#include "global.hpp"

#include <atomic>
#include "numtype.h"

namespace Global
{
namespace Stats
{
std::atomic<uint64_t> split_count(0);
std::atomic<uint64_t> microsurface_count(0);
std::atomic<uint64_t> microelement_count(0);
std::atomic<uint64_t> microelement_min_count(99999999999999);
std::atomic<uint64_t> microelement_max_count(0);
std::atomic<uint64_t> cache_misses(0);
std::atomic<size_t> primitive_ray_tests(0);

std::atomic<uint64_t> nan_count(0);
std::atomic<uint64_t> inf_count(0);
} // Stats
} // Global
