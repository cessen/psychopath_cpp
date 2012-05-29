#ifndef TRIANGLE_HPP
#define TRIANGLE_HPP

#include "numtype.h"

#include <stdlib.h>
#include "vector.hpp"

/* Not a true primitive.  Mainly a utility for tracing upoly grids.
 */
struct UTriangle {
	Vec3 verts[3];

	UTriangle() {}
	UTriangle(const Vec3 &v1, const Vec3 &v2, const Vec3 &v3);

	bool intersect_ray_(const Ray &ray,
	                    float32 *t=NULL, float32 *u=NULL, float32 *v=NULL) const;
};

#endif
