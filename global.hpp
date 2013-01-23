#ifndef PSYCHO_GLOBAL_HPP
#define PSYCHO_GLOBAL_HPP

#include <atomic>
#include "numtype.h"

namespace Global
{
namespace Stats
{
extern std::atomic<uint64> split_count;
extern std::atomic<uint64> microsurface_count;
extern std::atomic<uint64> microelement_count;
extern std::atomic<uint64> microelement_min_count;
extern std::atomic<uint64> microelement_max_count;
extern std::atomic<uint64> cache_misses;
extern std::atomic<uint_i> primitive_ray_tests;
} // Stats
} // Global

#endif // PSYCHO_GLOBAL_HPP