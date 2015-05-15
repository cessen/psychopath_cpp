#ifndef PSYCHO_GLOBAL_HPP
#define PSYCHO_GLOBAL_HPP

#include <atomic>
#include "numtype.h"

//#define GLOBAL_STATS_TOP_LEVEL_BVH_NODE_TESTS

namespace Global
{
extern std::atomic<size_t> next_object_uid;

namespace Stats
{
extern std::atomic<uint64_t> rays_shot;
extern std::atomic<uint64_t> split_count;
extern std::atomic<size_t> object_ray_tests;
extern std::atomic<size_t> top_level_bvh_node_tests;

extern std::atomic<uint64_t> nan_count;
extern std::atomic<uint64_t> inf_count;

static void clear()
{
	rays_shot = 0;
	split_count = 0;
	object_ray_tests = 0;
	top_level_bvh_node_tests = 0;

	nan_count = 0;
	inf_count = 0;
}

} // Stats
} // Global

#endif // PSYCHO_GLOBAL_HPP