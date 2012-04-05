#include "config.h"

namespace Config
{
    float dice_rate = 0.7; // 0.7 is about half pixel area
    float focus_factor = 0.707;
    float min_upoly_size = 0.01; // Approximate minimum micropolygon size in world space
    int max_grid_size = 32;
    int cache_size = 30; // In MB
    
    // Statistics
    unsigned long split_count = 0;
    unsigned long upoly_gen_count = 0;
    unsigned long cache_misses = 0;
}
