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
	bool hit;
	bool backfacing;

	// Information about the intersection point
	float32 t; // T-parameter along the ray at the intersection
	Vec3 p;  // Intersection postion


	// Information about the ray that caused the intersection
	Vec3 in; // The incoming ray direction
	float32 ow, dw;
	//Vec3 odx, ody; // Origin differentials
	//Vec3 ddx, ddy; // Direction differentials

	// Information about the surface normal at the point
	Vec3 n;
	//Vec3 ndx, ndy;

	// Information about the UVs at the point
	float32 u, v;
	//float32 udx, vdx;
	//float32 udy, vdy;

	// Offset for subsequent spawned rays to avoid self-intersection
	// Should be added for reflection, subtracted for transmission
	Vec3 offset;

	Color col;

	Intersection(): hit {false}, t {std::numeric_limits<float32>::infinity()} {}


	/**
	 * @brief Gets the projection of the ray x differentials onto
	 * the intersection.
	 */
	/*Vec3 pdx() const {
		const float32 dn = dot(in, n);
		if (dn < DIFFERENTIAL_DOT_EPSILON && dn > -DIFFERENTIAL_DOT_EPSILON) {
			std::cout << "YAR\n";
			return in.normalized() * (odx + (ddx * t)).length();
		} else {
			const Vec3 temp = odx + (ddx * t);
			const float32 td = dot(temp, n) / dn;
			return temp + (in * td);
		}
	}*/

	/**
	 * @brief Gets the projection of the ray y differentials onto
	 * the intersection.
	 */
	/*Vec3 pdy() const {
		const float32 dn = dot(in, n);
		if (dn < DIFFERENTIAL_DOT_EPSILON && dn > -DIFFERENTIAL_DOT_EPSILON) {
			return cross(n, in).normalized() * (ody + (ddy * t)).length();
		} else {
			const Vec3 temp = ody + (ddy * t);
			const float32 td = dot(temp, n) / dn;
			return temp + (in * td);
		}
	}*/

	/**
	 * @brief Returns the ray width at the intersection point.
	 */
	float32 owp() const {
		return ow + (dw * t);
	}

};

#endif
