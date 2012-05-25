#include "numtype.h"

#include "config.h"

namespace Config
{
    float32 dice_rate = 0.7; // 0.7 is about half pixel area
    float32 focus_factor = 0.707;
    float32 min_upoly_size = 0.01; // Approximate minimum micropolygon size in world space
    int max_grid_size = 32;
    int cache_size = 30; // In MB
    
    // Statistics
    uint64 split_count = 0;
    uint64 upoly_gen_count = 0;
    uint64 cache_misses = 0;
}

