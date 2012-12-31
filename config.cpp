#include "numtype.h"

#include "config.hpp"

namespace Config
{
float32 dice_rate = 1.0; // 0.7 is about half pixel area
float32 min_upoly_size = 0.001; // Approximate minimum micropolygon size in world space
uint_i max_grid_size = 32;
float grid_cache_size = 30.0; // In MB

// Statistics
uint64 split_count = 0;
uint64 microsurface_count = 0;
uint64 microelement_count = 0;
extern uint64 microelement_min_count = 99999999999999;
extern uint64 microelement_max_count = 0;

uint64 cache_misses = 0;
uint_i primitive_ray_tests = 0;
}

