#ifndef INTERSECTION_HPP
#define INTERSECTION_HPP

#include "numtype.h"

#include "vector.hpp"
#include "color.hpp"

/*
 * Contains the information from a ray intersection.
 */
struct Intersection {
	// Information about the intersection
	Vec3 p;  // Intersection postion
	Vec3 n;  // Surface normal at the intersection

	// Information about the ray that caused the intersection
	Vec3 in; // The incoming ray direction
	float32 t; // T-parameter along the ray at the intersection

	// Offset for subsequent spawned rays to avoid self-intersection
	// Should be added for reflection, subtracted for transmission
	Vec3 offset;

	Color col;
};

#endif
