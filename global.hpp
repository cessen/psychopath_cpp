#ifndef PSYCHO_GLOBAL_HPP
#define PSYCHO_GLOBAL_HPP

#include "numtype.h"

namespace Global
{
namespace Stats
{
extern uint64 split_count;
extern uint64 microsurface_count;
extern uint64 microelement_count;
extern uint64 microelement_min_count;
extern uint64 microelement_max_count;
extern uint64 cache_misses;
extern uint_i primitive_ray_tests;
} // Stats
} // Global

#endif // PSYCHO_GLOBAL_HPP