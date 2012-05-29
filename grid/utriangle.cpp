/*
 * Triangle intersection code by Tomas Moller, May 2000
 * http://www.acm.org/jgt/
 *
 * Modified, Nathan Vegdahl
 */

#include "numtype.h"

#include "vector.h"
#include "ray.hpp"
#include "utriangle.hpp"
#include <math.h>
#include <iostream>

#define EPSILON 0.000001

UTriangle::UTriangle(const Vec3 &v1, const Vec3 &v2, const Vec3 &v3)
{
	verts[0] = v1;
	verts[1] = v2;
	verts[2] = v3;
}

bool UTriangle::intersect_ray_(const Ray &ray,
                               float32 *t_, float32 *u_, float32 *v_) const
{
	Vec3 qvec;
	float32 u, v;

	/* find vectors for two edges sharing vert0 */
	const Vec3 edge1 = verts[1] - verts[0];
	const Vec3 edge2 = verts[2] - verts[0];

	/* begin calculating determinant - also used to calculate U parameter */
	const Vec3 pvec = cross(ray.d, edge2);

	/* if determinant is near zero, ray lies in plane of triangle */
	const float32 det = dot(edge1, pvec);

	/* calculate distance from vert0 to ray origin */
	const Vec3 tvec = ray.o - verts[0];
	const float32 inv_det = 1.0 / det;

	if (det > EPSILON) {
		/* calculate U parameter and test bounds */
		u = dot(tvec, pvec);

		if (u < 0.0 || u > det)
			return false;

		/* prepare to test V parameter */
		qvec = cross(tvec, edge1);

		/* calculate V parameter and test bounds */
		v = dot(ray.d, qvec);
		if (v < 0.0 || u + v > det)
			return false;
	} else if (det < -EPSILON) {
		/* calculate U parameter and test bounds */
		u = dot(tvec, pvec);
		if (u > 0.0 || u < det)
			return false;

		/* prepare to test V parameter */
		qvec = cross(tvec, edge1);

		/* calculate V parameter and test bounds */
		v = dot(ray.d, qvec);
		if (v > 0.0 || u + v < det)
			return false;
	} else
		return false;  /* ray is parallell to the plane of the triangle */

	/* calculate t, ray intersects triangle */
	//*t = DOT(edge2, qvec) * inv_det;
	if (t_)
		(*t_) = dot(edge2, qvec) * inv_det;
	if (u_)
		(*u_) = u * inv_det;
	if (v_)
		(*v_) = v * inv_det;

	return true;
}

