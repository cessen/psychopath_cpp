#include "global.hpp"

#include <atomic>
#include "numtype.h"

namespace Global
{
std::atomic<size_t> next_object_uid {0};

namespace Stats
{
std::atomic<uint64_t> rays_shot(0);
std::atomic<uint64_t> split_count(0);
std::atomic<size_t> object_ray_tests(0);
std::atomic<size_t> top_level_bvh_node_tests(0);

std::atomic<uint64_t> nan_count(0);
std::atomic<uint64_t> inf_count(0);
} // Stats
} // Global
