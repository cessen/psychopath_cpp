#include "numtype.h"

#include "config.hpp"

namespace Config {
bool no_output = false; // Suppress writing the image to disk, for better timing tests without as much I/O latency
float dice_rate = 0.25; // 0.7 is about half pixel area
float min_upoly_size = 0.00001; // Approximate minimum micropolygon size in world space
uint8_t max_grid_size = 16;
float grid_cache_size = 64.0; // In MB

int samples_per_bucket = 1 << 16; // The number of samples to aim to take per-bucket (used in auto-sizing buckets)

float displace_distance = 0.00f;
}
