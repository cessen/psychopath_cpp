#ifndef TRIANGLE_HPP
#define TRIANGLE_HPP

#include "numtype.h"

#include <stdlib.h>
#include "vector.hpp"

/**
 * Not a true primitive.  Mainly a utility for tracing upoly grids.
 */
struct UTriangle {
	Vec3 verts[3];

	UTriangle() {}
	UTriangle(const Vec3 &v1, const Vec3 &v2, const Vec3 &v3);

	/**
	 * @brief Ray/triangle intersection test.
	 *
	 * If the ray intersects, then the t, u, and v parameters are filled in
	 * with the appropriate information.  Note that this does _not_ check
	 * to see if the hit is within the ray's [min_t, max_t] extents, and
	 * will return true even if the intersection is outside of those
	 * extents.
	 *
	 * @param ray The ray to test again.
	 * @param[out] t The t param along the ray where the intersection occured.
	 * @param[out] u The u parameter on the triangle where the interesection occured.
	 * @param[out] v The v parameter on the triangle where the interesection occured.
	 *
	 * @returns True if the ray intersects the triangle, false if not.
	 */
	bool intersect_ray_(const Ray &ray,
	                    float32 *t=NULL, float32 *u=NULL, float32 *v=NULL) const;
};

#endif
