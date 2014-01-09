#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "numtype.h"

namespace Config
{
extern bool no_output;
extern float dice_rate;
extern float min_upoly_size;
extern uint8_t max_grid_size;
extern float grid_cache_size;

extern int samples_per_bucket;

extern float displace_distance;
}

#endif
