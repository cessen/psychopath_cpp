#ifndef POTENTIALINTER_HPP
#define POTENTIALINTER_HPP

#include "numtype.h"

/**
 * @brief Records information about a potential intersection with an object.
 */
struct PotentialInter {
	bool valid; // The potential intersection data is filled and valid
	uint_i object_id;
	uint_i ray_index;
	float32 nearest_hit_t; // The nearest possible hit distance along the ray
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
