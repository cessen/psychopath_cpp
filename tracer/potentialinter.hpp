#ifndef POTENTIALINTER_HPP
#define POTENTIALINTER_HPP

#include "numtype.h"
#include "rayinter.hpp"

/**
 * @brief Records information about a potential intersection with an object.
 */
struct PotentialInter {
	uint64 object_id;
	RayInter *ray_inter;
	float32 nearest_hit_t; // The nearest possible hit distance along the ray
	byte pad[8];
};

#endif // POTENTIALINTER_HPP
