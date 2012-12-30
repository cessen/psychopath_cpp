#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "numtype.h"

namespace Config
{
extern float32 dice_rate;
extern float32 focus_factor;
extern float32 min_upoly_size;
extern int max_grid_size;
extern float grid_cache_size;

// Statistics
extern uint64 split_count;
extern uint64 microelement_gen_count;
extern uint64 cache_misses;
extern uint_i primitive_ray_tests;

extern uint_i grid_size_accum;
extern uint_i grid_count;
}

#endif
