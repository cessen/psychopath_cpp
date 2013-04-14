#ifndef RAY_HPP
#define RAY_HPP

#include "numtype.h"

#include "vector.hpp"
#include "matrix.hpp"
#include "transform.hpp"
#include "config.hpp"
#include <algorithm>
#include <limits>
#include <math.h>
#include <iostream>
#include <assert.h>

/*
 * Transfer's a ray differential onto a surface intersection.
 * This assumes that both normal and d are normalized.
 *
 * normal is the surface normal at the intersection
 * d is the primary ray's direction
 * t is the distance along the primary ray to the intersection
 * od is the ray origin differential
 * dd is the ray direction differential
 *
 * Returns the origin differential transfered onto the surface intersection.
 */
/*static inline Vec3 transfer_ray_origin_differential(const Vec3 normal, const Vec3 d, const float t,
        const Vec3 od, const Vec3 dd)
{
	const Vec3 temp = od + (dd * t);
	const float td = dot(temp, normal) / dot(d, normal);

	return temp + (d * td);
}*/



/*
 * A ray in 3d space.
 * Includes information about ray differentials.
 */
struct Ray {
	// Coordinates
	Vec3 o, d; // Ray origin and direction
	float time; // Time coordinate

	// Minimum and maximum extent along the ray
	float min_t;
	float max_t;

	float ow; // Origin width
	float dw; // Width delta

	/*
	// Ray differentials for origin and direction
	Vec3 odx, ody; // Ray origin
	Vec3 ddx, ddy; // Ray direction

	// Rates of ray differentials' change, as a function of distance
	// along the ray.  This is useful for determining if
	// the range of micropolygon sizes across a surface
	// is going to be too broad for dicing.
	float diff_rate_x, diff_rate_y;

	// Whether the ray has differentials or not
	bool has_differentials;*/

	// Shadow ray or not
	bool is_shadow_ray;

	// Pre-computed data for accelerating ray intersection
	Vec3 inv_d;                 // 1.0/d
	uint32_t d_is_neg[3];  // Whether each component of d is negative


	/*
	 * Constructor.
	 * Ray differentials need to be filled in manually after this.
	 */
	Ray(const Vec3 &o_=Vec3(0.0f,0.0f,0.0f), const Vec3 &d_=Vec3(0.0f,0.0f,0.0f),
	    const float &time_ = 0.0f):
		o {o_},
	  d {d_},
	  time {time_},
	  min_t {0.0f},
	  max_t {std::numeric_limits<float>::infinity()},
	  //has_differentials {false},
	  is_shadow_ray {false}
	{}


	/**
	 * Computes the acceleration data for speedy bbox intersection testing.
	 */
	void update_accel() {
		inv_d = Vec3(1.0f, 1.0f, 1.0f) / d;

		d_is_neg[0] = d.x < 0 ? 1 : 0;
		d_is_neg[1] = d.y < 0 ? 1 : 0;
		d_is_neg[2] = d.z < 0 ? 1 : 0;
	}

	/**
	 * Pre-computes some useful data about the ray differentials.
	 */
	/*void update_differentials() {
		if (has_differentials) {
			diff_rate_x = ddx.length();
			diff_rate_y = ddy.length();
		}
	}*/


	/*
	 * Finalizes the ray after first initialization.
	 * Should only be called once, prior to tracing with the ray.
	 */
	void finalize() {
		// TODO: will normalizing things here mess anything up elsewhere?
		assert(d.length() > 0.0f);
		float linv = 1.0f / d.length();
		d.normalize();

		// Adjust the ray differentials for the normalized ray
		dw *= linv;
		/*if (has_differentials) {
			odx *= linv;
			ody *= linv;
			ddx *= linv;
			ddy *= linv;
		}*/

		update_accel();
		//update_differentials();
	}

	/**
	 * Applies a Transform.
	 */
	void apply_transform(const Transform &t) {
		// Origin and direction
		o = t.pos_to(o);
		d = t.dir_to(d);

		// Differentials
		// These can be transformed as directional vectors...?
		/*if (has_differentials) {
			odx = t.dir_to(odx);
			ody = t.dir_to(ody);
			ddx = t.dir_to(ddx);
			ddy = t.dir_to(ddy);
		}*/

		update_accel();
		//update_differentials();
	}

	/**
	 * Applies a Transform's inverse.
	 */
	void reverse_transform(const Transform &t) {
		// Origin and direction
		o = t.pos_from(o);
		d = t.dir_from(d);

		// Differentials
		// These can be transformed as directional vectors...?
		/*if (has_differentials) {
			odx = t.dir_from(odx);
			ody = t.dir_from(ody);
			ddx = t.dir_from(ddx);
			ddy = t.dir_from(ddy);
		}*/

		update_accel();
		//update_differentials();
	}


	/*
	 * Transfers all ray origin differentials to the surface
	 * intersection.
	 *
	 * Returns true on success.
	 */
	/*bool transfer_ray_differentials(const Vec3 normal, const float t) {
		if (!has_differentials)
			return false;

		const float d_n = dot(d, normal);

		if (d_n == 0.0f)
			return false;

		// x
		const Vec3 tempx = odx + (ddx * t);
		const float tdx = dot(tempx, normal) / d_n;
		odx = tempx + (d * tdx);

		// y
		const Vec3 tempy = ody + (ddy * t);
		const float tdy = dot(tempy, normal) / d_n;
		ody = tempy + (d * tdy);

		return true;
	}*/


	/*
	 * Returns the "ray width" at the given distance along the ray.
	 * The values returned corresponds to roughly the width that a micropolygon
	 * needs to be for this ray at that distance.  And that is its primary
	 * purpose as well: determining dicing rates.
	 */
	float width(const float &t) const {
		/*if (!has_differentials)
			return 0.0f;

		// Calculate the width of each differential at t.
		const Vec3 rev_d = d * -1.0f;
		const float d_n = dot(d, rev_d);
		float wx, wy;

		// x
		const Vec3 tempx = odx + (ddx * t);
		const float tdx = dot(tempx, rev_d) / d_n;
		wx = (tempx + (d * tdx)).length();

		// y
		const Vec3 tempy = ody + (ddy * t);
		const float tdy = dot(tempy, rev_d) / d_n;
		wy = (tempy + (d * tdy)).length();

		// Smaller of x and y
		return std::min(wx, wy) * 0.5f;*/

		return ow + (dw * t);
	}

	/*
	 * Returns an estimate of the minimum ray width over a distance
	 * range along the ray.
	 */
	float min_width(const float &tnear, const float &tfar) const {
		/*if (!has_differentials) {
			return 0.0f;
			std::cout << "YAR\n";
		}

		// Calculate the min width of each differential at the average distance.
		const float t = (tnear + tfar) / 2.0f;
		const Vec3 rev_d = d * -1.0f;
		const float d_n = dot(d, rev_d);
		float wx, wy;

		// x
		const Vec3 tempx = odx + (ddx * t);
		const float tdx = dot(tempx, rev_d) / d_n;
		wx = (tempx + (d * tdx)).length() - (diff_rate_x * 0.5);
		if (wx < 0.0f)
			wx = 0.0f;

		// y
		const Vec3 tempy = ody + (ddy * t);
		const float tdy = dot(tempy, rev_d) / d_n;
		wy = (tempy + (d * tdy)).length() - (diff_rate_y * 0.5);
		if (wy < 0.0f)
			wy = 0.0f;

		// Minimum of x and y
		const float result = std::min(wx, wy) * 0.5f;

		return result;*/

		return ow + (dw * tnear);
	}

};







#endif
