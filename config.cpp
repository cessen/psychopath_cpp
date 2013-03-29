#include "numtype.h"

#include "config.hpp"

namespace Config
{
float32 dice_rate = 1.0; // 0.7 is about half pixel area
float32 min_upoly_size = 0.001; // Approximate minimum micropolygon size in world space
uint_i max_grid_size = 64;
float32 grid_cache_size = 128.0; // In MB

float32 displace_distance = 0.15f;
}
