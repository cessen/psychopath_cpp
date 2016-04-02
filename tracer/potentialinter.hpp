#ifndef POTENTIALINTER_HPP
#define POTENTIALINTER_HPP

#include "numtype.h"

/**
 * @brief Records information about a potential intersection with an object.
 */
struct PotentialInter {
	size_t object_id;
	size_t ray_index;
	float nearest_hit_t; // The nearest possible hit distance along the ray
	bool valid; // The potential intersection data is filled and valid
	uint8_t tag; // Used for misc purposes

	bool operator<(const PotentialInter &b) const {
		return object_id < b.object_id;
	}
};

static bool compare_potint(const PotentialInter &a, const PotentialInter &b) {
	// Sort by object id
	return a.object_id < b.object_id;
}

static size_t index_potint(const PotentialInter &a) {
	return a.object_id;
}

#endif // POTENTIALINTER_HPP
