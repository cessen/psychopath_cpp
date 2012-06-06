#ifndef INTERSECTION_HPP
#define INTERSECTION_HPP

#include "numtype.h"

#include "vector.hpp"
#include "color.hpp"

/*
 * Contains the information from a ray intersection.
 */
struct Intersection {
	Vec3 p;  // Intersection postion
	Vec3 n;  // Surface normal at the intersection
	float32 t; // T-parameter along the ray at the intersection

	Color col;
};

#endif
