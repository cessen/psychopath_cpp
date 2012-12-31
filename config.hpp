#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "numtype.h"

namespace Config
{
extern float32 dice_rate;
extern float32 focus_factor;
extern float32 min_upoly_size;
extern uint_i max_grid_size;
extern float grid_cache_size;

// Statistics
extern uint64 split_count;
extern uint64 microsurface_count;
extern uint64 microelement_count;
extern uint64 cache_misses;
extern uint_i primitive_ray_tests;
}

#endif
