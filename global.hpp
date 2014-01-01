#ifndef PSYCHO_GLOBAL_HPP
#define PSYCHO_GLOBAL_HPP

#include <atomic>
#include "numtype.h"

namespace Global
{
extern std::atomic<size_t> next_primitive_id;

namespace Stats
{
extern std::atomic<uint64_t> split_count;
extern std::atomic<uint64_t> microsurface_count;
extern std::atomic<uint64_t> microelement_count;
extern std::atomic<uint64_t> microelement_min_count;
extern std::atomic<uint64_t> microelement_max_count;
extern std::atomic<uint64_t> cache_misses;
extern std::atomic<size_t> primitive_ray_tests;

extern std::atomic<uint64_t> nan_count;
extern std::atomic<uint64_t> inf_count;
} // Stats
} // Global

#endif // PSYCHO_GLOBAL_HPP