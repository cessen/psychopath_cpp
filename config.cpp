#include "numtype.h"

#include "config.hpp"

namespace Config
{
float dice_rate = 0.5; // 0.7 is about half pixel area
float min_upoly_size = 0.001; // Approximate minimum micropolygon size in world space
uint8_t max_grid_size = 128;
float grid_cache_size = 256.0; // In MB

int samples_per_bucket = 250000; // The number of samples to aim to take per-bucket (used in auto-sizing buckets)

float displace_distance = 0.00f;
}
