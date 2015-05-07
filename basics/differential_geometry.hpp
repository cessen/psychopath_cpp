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

		//geo.n = xform.nor_from(n).normalized();
		//const Vec3 nu = xform.nor_from(n+dndu).normalized();
		//const Vec3 nv = xform.nor_from(n+dndv).normalized();
		//geo.dndu = nu - geo.n;
		//geo.dndv = nv - geo.n;

		geo.n = xform.dir_from(n);
		geo.dndu = xform.dir_from(dndu);
		geo.dndv = xform.dir_from(dndv);
		const float il = 1.0f / geo.n.length();
		geo.n *= il;
		geo.dndu *= il;
		geo.dndv *= il;

		return geo;
	}


	DifferentialGeometry transformed_to(const Transform& xform) const {
		DifferentialGeometry geo;
		geo.u = u;
		geo.v = v;

		geo.p = xform.pos_to(p);
		geo.dpdu = xform.dir_to(dpdu);
		geo.dpdv = xform.dir_to(dpdv);

		//geo.n = xform.nor_to(n).normalized();
		//const Vec3 nu = xform.nor_to(n+dndu).normalized();
		//const Vec3 nv = xform.nor_to(n+dndv).normalized();
		//geo.dndu = nu - geo.n;
		//geo.dndv = nv - geo.n;

		geo.n = xform.dir_to(n);
		geo.dndu = xform.dir_to(dndu);
		geo.dndv = xform.dir_to(dndv);
		const float il = 1.0f / geo.n.length();
		geo.n *= il;
		geo.dndu *= il;
		geo.dndv *= il;

		return geo;
	}


	void flip_normal() {
		n *= -1.0f;
		dndu *= -1.0f;
		dndv *= -1.0f;
	}
};

/*
 * Transfers a ray differential onto a surface intersection.
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
	const float td = -dot(temp, normal) / dot(d, normal);

	return temp + (d * td);
}


/*
* Reflects a ray differential off of a surface intersection as a
* perfect mirror.
* This assumes that 'normal' is normalized.
*
* normal is the surface normal at the intersection
* normal_d is the surface normal differential for the intersection
* d is the primary ray's direction
* dd is the ray direction differential
*
* Returns the direction differential reflected off the surface.
*/
static inline Vec3 reflect_ray_direction_differential(const Vec3 normal, const Vec3 normal_d, const Vec3 d, const Vec3 dd)
{
	const auto ddn = dot(dd, normal) + dot(d, normal_d);
	const auto tmp = (normal_d * dot(d, normal)) + (normal * ddn);
	return dd - (tmp * 2.0f);
}


/**
 * Clamps the direction differentials of a ray to not have slopes
 * exceeding 1.0.  This is important to prevent self-intersections with
 * micro-geometry.
 */
static inline void clamp_dd(WorldRay* ray)
{
	const float len_d = ray->d.length();
	const float len_dx = ray->ddx.length();
	const float len_dy = ray->ddy.length();

	if ((len_dx / len_d) > 0.9f)
		ray->ddx *= 0.9f * len_d / len_dx;

	if ((len_dy / len_d) > 0.9f)
		ray->ddy *= 0.9f * len_d / len_dy;
}


/**
 * Calculates the uv coordinate differentials at the given differential
 * hit point.
 *
 * TODO: apparently this is wrong.  See pg. 508 of PBRT for a correct
 * implementation.
 */
static inline std::pair<float, float> calc_uv_differentials(const Vec3 dp, const Vec3 dpdu, const Vec3 dpdv)
{
	const float dudp = dot(dp, dpdu);
	const float dvdp = dot(dp, dpdv);

	return std::make_pair(dudp, dvdp);
}

#endif // DIFFERENTIAL_GEOMETRY_HPP