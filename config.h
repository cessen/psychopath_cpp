#ifndef CONFIG_H
#define CONFIG_H

namespace Config
{
    extern float dice_rate;
    extern float focus_factor;
    extern float min_upoly_size;
    extern int max_grid_size;
    extern int grid_cache_size;
    
    // Statistics
    extern unsigned long split_count;
    extern unsigned long upoly_gen_count;
    extern unsigned long cache_misses;
}

#endif
