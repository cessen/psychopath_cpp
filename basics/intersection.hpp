#ifndef INTERSECTION_HPP
#define INTERSECTION_HPP

#include "numtype.h"

#include <limits>

#include "vector.hpp"
#include "color.hpp"

#define DIFFERENTIAL_DOT_EPSILON 0.0000f

/*
 * Contains the information from a ray intersection.
 */
struct Intersection {
	// Whether there's a hit or not
	bool hit {false};
	bool backfacing {false};

	// Information about the intersection point
	float t {std::numeric_limits<float>::infinity()}; // T-parameter along the ray at the intersection
	Vec3 p {0.0f, 0.0f, 0.0f};  // Intersection postion


	// Information about the ray that caused the intersection
	Vec3 in {0.0f, 0.0f, 0.0f}; // The incoming ray direction
	float ow {0.0f}, dw {0.0f};
	//Vec3 odx, ody; // Origin differentials
	//Vec3 ddx, ddy; // Direction differentials

	// Information about the surface normal at the point
	Vec3 n {0.0f, 0.0f, 0.0f};
	//Vec3 ndx, ndy;

	// Information about the UVs at the point
	float u {0.0f}, v {0.0f};
	//float udx, vdx;
	//float udy, vdy;

	// Offset for subsequent spawned rays to avoid self-intersection
	// Should be added for reflection, subtracted for transmission
	Vec3 offset {0.0f, 0.0f, 0.0f};

	Color col {0.0f};


	/**
	 * @brief Gets the projection of the ray x differentials onto
	 * the intersection.
	 */
	/*Vec3 pdx() const {
		const float dn = dot(in, n);
		if (dn < DIFFERENTIAL_DOT_EPSILON && dn > -DIFFERENTIAL_DOT_EPSILON) {
			std::cout << "YAR\n";
			return in.normalized() * (odx + (ddx * t)).length();
		} else {
			const Vec3 temp = odx + (ddx * t);
			const float td = dot(temp, n) / dn;
			return temp + (in * td);
		}
	}*/

	/**
	 * @brief Gets the projection of the ray y differentials onto
	 * the intersection.
	 */
	/*Vec3 pdy() const {
		const float dn = dot(in, n);
		if (dn < DIFFERENTIAL_DOT_EPSILON && dn > -DIFFERENTIAL_DOT_EPSILON) {
			return cross(n, in).normalized() * (ody + (ddy * t)).length();
		} else {
			const Vec3 temp = ody + (ddy * t);
			const float td = dot(temp, n) / dn;
			return temp + (in * td);
		}
	}*/

	/**
	 * @brief Returns the ray width at the intersection point.
	 */
	float owp() const {
		return ow + (dw * t);
	}

};

#endif
