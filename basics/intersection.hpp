#ifndef INTERSECTION_HPP
#define INTERSECTION_HPP

#include "numtype.h"

#include <limits>

#include "transform.hpp"
#include "vector.hpp"
#include "color.hpp"
#include "differential_geometry.hpp"

#define DIFFERENTIAL_DOT_EPSILON 0.0000f

/*
 * Contains the information from a ray intersection.
 */
struct Intersection {
	// Whether there's a hit or not
	bool hit {false};

	// The space that the intersection took place in, relative to world space.
	Transform space;

	// Information about the intersection point
	float t {std::numeric_limits<float>::infinity()}; // T-parameter along the ray at the intersection
	bool backfacing {false}; // Whether it hit the backface of the surface
	float light_pdf {1.0f};  // Pdf of selecting this hit point and ray via light sampling

	// Differential geometry at the hit point
	DifferentialGeometry geo;

	// Offset for subsequent spawned rays to avoid self-intersection
	// Should be added for reflection, subtracted for transmission
	Vec3 offset {0.0f, 0.0f, 0.0f};

	Color col {0.0f};
};

#endif
