#ifndef POTENTIALINTER_HPP
#define POTENTIALINTER_HPP

#include "numtype.h"
#include "rayinter.hpp"

/**
 * @brief Records information about a potential intersection with an object.
 */
struct PotentialInter {
	uint_i object_id;
	RayInter *ray_inter;
	float32 nearest_hit_t; // The nearest possible hit distance along the ray
	byte pad[8];
};

static bool compare_potint(const PotentialInter &a, const PotentialInter &b)
{
	// Sort by object id
	return a.object_id < b.object_id;
}

static uint_i index_potint(const PotentialInter &a)
{
	return a.object_id;
}

#endif // POTENTIALINTER_HPP
