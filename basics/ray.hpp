#ifndef RAY_HPP
#define RAY_HPP

#include "numtype.h"

#include "vector.hpp"
#include "matrix.hpp"
#include "config.hpp"
#include <algorithm>
#include <math.h>
#include <iostream>
#include <assert.h>


#define NUM_DIFFERENTIALS 4

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
/*
static inline Vec3 transfer_differential(const Vec3 normal, const Vec3 d, const float32 t,
                                          const Vec3 od, const Vec3 dd)
{
    const Vec3 temp = od + (dd * t);
    const float32 td = dot(temp, normal) / dot(d, normal);

    return temp + (d * td);
}
*/



/*
 * A ray in 3d space.
 * Includes information about ray differentials.
 */
struct Ray {
	// Coordinates
	Vec3 o, d; // Ray origin and direction
	float32 time; // Time coordinate

	// Minimum and maximum extent along the ray
	float32 min_t;
	float32 max_t;

	// Shadow ray or not
	bool is_shadow_ray;

	// Differentials for origin and direction
	// 0: Image X
	// 1: Image Y
	// 2: Lens X
	// 3: Lens Y
	Vec3 od[NUM_DIFFERENTIALS]; // Ray origin differentials
	Vec3 dd[NUM_DIFFERENTIALS]; // Ray direction differentials

	bool has_differentials;

	// Pre-computed data for accelerating ray intersection
	Vec3 inv_d;                 // 1.0/d
	uint32 d_is_neg[3];  // Whether each component of d is negative

	// Rates of ray differentials' change, as a function of distance
	// along the ray.  This is useful for determining if
	// the range of micropolygon sizes across a surface
	// is going to be too broad for dicing.
	float32 diff_rate[NUM_DIFFERENTIALS];



	/*
	 * Constructor.
	 * Ray differentials need to be filled in manually after this.
	 */
	Ray(const Vec3 &o_=Vec3(0,0,0), const Vec3 &d_=Vec3(0,0,0),
	    const float32 &time_ = 0.0) {
		o = o_;
		d = d_;

		time = time_;

		min_t = 0.0;
		max_t = 99999999999999999999999999.0;

		is_shadow_ray = false;
		has_differentials = false;
	}


	/**
	 * Computes the acceleration data for speedy bbox intersection testing.
	 */
	void update_accel() {
		inv_d.x = 1.0 / d.x;
		inv_d.y = 1.0 / d.y;
		inv_d.z = 1.0 / d.z;

		d_is_neg[0] = d.x < 0;
		d_is_neg[1] = d.y < 0;
		d_is_neg[2] = d.z < 0;
	}

	/**
	 * Pre-computes some useful data about the ray differentials.
	 */
	void update_differentials() {
		if (has_differentials) {
			for (int32 i = 0; i < NUM_DIFFERENTIALS; i++) {
				diff_rate[i] = dd[i].length();
			}
		}
	}


	/*
	 * Finalizes the ray after first initialization.
	 * Should only be called once, prior to tracing with the ray.
	 */
	void finalize() {
		// TODO: will normalizing things here mess anything up elsewhere?
		assert(d.length() > 0.0);
		float32 linv = 1.0 / d.normalize();

		// Adjust the ray differentials for the normalized ray
		if (has_differentials) {
			for (int32 i = 0; i < NUM_DIFFERENTIALS; i++) {
				od[i] = od[i] * linv;
				dd[i] = dd[i] * linv;
			}
		}

		update_accel();
		update_differentials();
	}


	/*
	 * Applies a matrix transform.
	 */
	void apply_matrix(const Matrix44 &m) {
		// Origin and direction
		o = m.mult_pos(o);
		d = m.mult_dir(d);

		// Differentials
		// These can be transformed as directional vectors...?
		if (has_differentials) {
			for (int32 i = 0; i < NUM_DIFFERENTIALS; i++) {
				od[i] = m.mult_dir(od[i]);
				dd[i] = m.mult_dir(dd[i]);
			}
		}

		update_accel();
		update_differentials();
	}


	/*
	 * Transfers all ray origin differentials to the surface
	 * intersection.
	 *
	 * Returns true on success.
	 */
	bool transfer_ray_differentials(const Vec3 normal, const float32 t) {
		if (!has_differentials)
			return false;

		const float32 d_n = dot(d, normal);

		if (d_n == 0.0)
			return false;

		for (int32 i = 0; i < NUM_DIFFERENTIALS; i++) {
			Vec3 temp = od[i] + (dd[i] * t);
			float32 td = dot(temp, normal) / d_n;

			od[i] = temp + (d * td);
		}

		return true;
	}


	/*
	 * Returns the "ray width" at the given distance along the ray.
	 * The values returned corresponds to roughly the width that a micropolygon
	 * needs to be for this ray at that distance.  And that is its primary
	 * purpose as well: determining dicing rates.
	 */
	float32 width(const float32 &t) const {
		if (!has_differentials)
			return 0.0f;

		// Calculate the width of each differential at t.
		const Vec3 rev_d = d * -1.0f;
		const float32 d_n = dot(d, rev_d);
		float32 w[NUM_DIFFERENTIALS];
		for (int32 i = 0; i < NUM_DIFFERENTIALS; i++) {
			Vec3 temp = od[i] + (dd[i] * t);
			float32 td = dot(temp, rev_d) / d_n;

			w[i] = (temp + (d * td)).length();
		}

		// Minimum of the image differentials
		const float32 image = std::min(w[0], w[1]);

		// Minimum of the lens differentials
		const float32 lens = std::min(w[2], w[3]);

		// Maximum of the results
		return std::max(image, lens) * 0.5f;
	}

	/*
	 * Returns an estimate of the minimum ray width over a distance
	 * range along the ray.
	 */
	float32 min_width(const float32 &tnear, const float32 &tfar) const {
		if (!has_differentials)
			return 0.0f;

		// Calculate the min width of each differential at the average distance.
		const float32 t = (tnear + tfar) / 2.0f;
		const Vec3 rev_d = d * -1.0f;
		const float32 d_n = dot(d, rev_d);
		float32 w[NUM_DIFFERENTIALS];
		for (int32 i = 0; i < NUM_DIFFERENTIALS; i++) {
			Vec3 temp = od[i] + (dd[i] * t);
			float32 td = dot(temp, rev_d) / d_n;

			w[i] = (temp + (d * td)).length() - (diff_rate[i] * 0.5);
			if (w[i] < 0.0f)
				w[i] = 0.0f;
		}

		// Minimum of the image differentials
		const float32 image = std::min(w[0], w[1]);

		// Minimum of the lens differentials
		const float32 lens = std::min(w[2], w[3]);

		//std::cout << "Image d: " << image << std::endl;
		//std::cout << "Lens d: " << lens << std::endl;

		// Maximum of the results
		return std::max(image, lens) * 0.5f;
	}

};







#endif
