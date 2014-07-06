#ifndef DIFFERENTIAL_GEOMETRY_HPP
#define DIFFERENTIAL_GEOMETRY_HPP

#include "numtype.h"
#include "vector.hpp"
#include "transform.hpp"

struct DifferentialGeometry {
	float u, v;

	// Point position
	Vec3 p;
	Vec3 dpdu, dpdv;

	// Surface normal
	Vec3 n;
	Vec3 dndu, dndv;


	DifferentialGeometry transformed_from(const Transform& xform) const {
		DifferentialGeometry geo;
		geo.u = u;
		geo.v = v;

		geo.p = xform.pos_from(p);
		geo.dpdu = xform.dir_from(dpdu);
		geo.dpdv = xform.dir_from(dpdv);

		geo.n = xform.nor_from(n).normalized();
		const Vec3 nu = xform.nor_from(n+dndu).normalized();
		const Vec3 nv = xform.nor_from(n+dndv).normalized();
		geo.dndu = nu - geo.n;
		geo.dndv = nv - geo.n;

		return geo;
	}


	DifferentialGeometry transformed_to(const Transform& xform) const {
		DifferentialGeometry geo;
		geo.u = u;
		geo.v = v;

		geo.p = xform.pos_to(p);
		geo.dpdu = xform.dir_to(dpdu);
		geo.dpdv = xform.dir_to(dpdv);

		geo.n = xform.nor_to(n).normalized();
		const Vec3 nu = xform.nor_to(n+dndu).normalized();
		const Vec3 nv = xform.nor_to(n+dndv).normalized();
		geo.dndu = nu - geo.n;
		geo.dndv = nv - geo.n;

		return geo;
	}


	void flip_normal() {
		n *= -1.0f;
		dndu *= -1.0f;
		dndv *= -1.0f;
	}
};

/*
 * Transfer's a ray differential onto a surface intersection.
 * This assumes that both normal and d are normalized.
 *
 * t is the distance along the primary ray to the intersection
 * normal is the surface normal at the intersection
 * d is the primary ray's direction
 * od is the ray origin differential
 * dd is the ray direction differential
 *
 * Returns the origin differential transfered onto the surface intersection.
 */
static inline Vec3 transfer_ray_origin_differential(const float t, const Vec3 normal, const Vec3 d,
        const Vec3 od, const Vec3 dd)
{
	const Vec3 temp = od + (dd * t);
	const float td = dot(temp, normal) / dot(d, normal);

	const Vec3 real_projected = temp + (d * td); // Real projected origin differential

	// Scaled for dicing rates
	return real_projected.normalized() * temp.length();
}

#endif // DIFFERENTIAL_GEOMETRY_HPP