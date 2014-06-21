#ifndef RAY_HPP
#define RAY_HPP

#include <array>
#include <algorithm>
#include <limits>
#include <math.h>
#include <iostream>
#include <assert.h>

#include "numtype.h"

#include "vector.hpp"
#include "matrix.hpp"
#include "transform.hpp"
#include "bit_stack.hpp"
#include "config.hpp"


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





/**
 * @brief A ray in 3d space.
 */
struct Ray {
	/**
	 * @brief Type to store ray direction signs in, and also ray bit flags.
	 * The ray direction signs are stored in indices 0-2, and the bit flags
	 * are in 3.
	 *
	 * It's typedef'd as it is for convenience.
	 */
	typedef std::array<uint8_t, 4> SignsAndFlags;  // Four slots instead of three for alignment
	typedef SignsAndFlags Signs;

	enum {
	    IS_OCCLUSION = 1 << 0, // Indicates that this is an occlusion ray
	    DONE = 1 << 1, // Indicates the ray is fully processed and can be ignored for any further traversal or testing
	    DEEPER_SPLIT = 1 << 2, // For traversing splittable surfaces
	    MISC5 = 1 << 3,
	    MISC4 = 1 << 4,
	    MISC3 = 1 << 5,
	    MISC2 = 1 << 6,
	    MISC1 = 1 << 7
	};

	// Ray data
	// Weird interleaving of fields is for alignment
	Vec3 o; // Origin
	float max_t; // Maximum extent along the ray
	Vec3 d; // Direction
	SignsAndFlags d_sign_and_flags; // Sign of the components of d in [0..2] and flags in [3]
	Vec3 d_inv; // 1.0 / d
	float time; // Time coordinate
	BitStack2<uint64_t> trav_stack;  // Bit stack used during BVH traversal
	float ow; // Origin width
	float dw; // Width delta
	uint32_t id;  // Ray id, used for indexing into other related structures




	/**
	 * @brief Constructor.
	 *
	 * Ray differentials need to be filled in manually after this.
	 *
	 * By default, origin (o) and direction (d) are initialized with NaN's.
	 */
	Ray(const Vec3 &o_ = Vec3(std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN()),
	    const Vec3 &d_ = Vec3(std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN()),
	    const float &time_ = 0.0f):
		o {o_},
	  max_t {std::numeric_limits<float>::infinity()},
	  d {d_},
	  d_sign_and_flags {0,0,0,0},
	  time {time_},
	  id {0}
	{}

	uint8_t& flags() {
		return d_sign_and_flags[3];
	}

	const uint8_t& flags() const {
		return d_sign_and_flags[3];
	}

	Vec3 get_d_inverse() const {
		return d_inv;
	}

	Signs get_d_sign() const {
		return d_sign_and_flags;
	}

	/**
	 * Computes the acceleration data for speedy bbox intersection testing.
	 */
	void update_accel() {
		// Inverse direction
		d_inv = Vec3(1.0f, 1.0f, 1.0f) / d;

		// Sign of the direction components
		d_sign_and_flags[0] = (d.x < 0.0f ? 1u : 0u);
		d_sign_and_flags[1] = (d.y < 0.0f ? 1u : 0u);
		d_sign_and_flags[2] = (d.z < 0.0f ? 1u : 0u);
	}


	/*
	 * Finalizes the ray after first initialization.
	 * Should only be called once, prior to tracing with the ray.
	 */
	void finalize() {
		assert(d.length() > 0.0f);

		update_accel();
	}

	/*
	 * Returns the "ray width" at the given distance along the ray.
	 * The values returned corresponds to roughly the width that a micropolygon
	 * needs to be for this ray at that distance.  And that is its primary
	 * purpose as well: determining dicing rates.
	 */
	float width(const float t) const {
		return ow + (dw * t);
	}

	/*
	 * Returns an estimate of the minimum ray width over a distance
	 * range along the ray.
	 */
	float min_width(const float tnear, const float tfar) const {
		return ow + (dw * tnear);
	}

};



/**
 * @brief A strictly world-space ray.
 */
struct WorldRay {
	/**
	 * @brief An enum that describes the type of a ray.
	 *
	 * The possible values are all powers of two, so that bitmasks
	 * can be easily created when tracking e.g. the types of rays
	 * in a path.
	 */
	enum Type: uint16_t {
	    NONE       = 0,
	    CAMERA     = 1 << 0,
	    R_DIFFUSE  = 1 << 1, // Diffuse reflection
	    R_SPECULAR = 1 << 2, // Specular reflection
	    T_DIFFUSE  = 1 << 3, // Diffuse transmission
	    T_SPECULAR = 1 << 4, // Specular transmission
	    OCCLUSION  = 1 << 5
	};

	Vec3 o, d; // Origin and direction
	Vec3 odx, ody; // Origin differentials
	Vec3 ddx, ddy; // Direction differentials

	float time;
	Type type;

	/**
	 * Returns a transformed version of the WorldRay.
	 */
	WorldRay transformed(const Transform& t) {
		WorldRay r = *this;

		r.o = t.pos_to(o);
		r.d = t.dir_to(d);

		r.odx = t.dir_to(odx);
		r.ody = t.dir_to(ody);
		r.ddx = t.dir_to(ddx);
		r.ddy = t.dir_to(ddy);

		return r;
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

	/**
	 * Modifies a Ray in-place to be consistent with the WorldRay.
	 */
	void update_ray(Ray* ray) const {
		Ray& r = *ray;

		// Origin, direction, and time
		r.o = o;
		r.d = d;

		// Translate differentials into ray width approximation
		// TODO: do this correctly for arbitrary ray differentials,
		// not just camera ray differentials.
		r.ow = std::min(odx.length(), ody.length());
		r.dw = std::min(ddx.length(), ddy.length());

		// Finalize ray
		r.finalize();
	}

	void update_ray(Ray* ray, const Transform& t) const {
		Ray& r = *ray;

		// Origin, direction, and time
		r.o = t.pos_to(o);
		r.d = t.dir_to(d);

		// Transformed ray differentials
		const Vec3 todx = t.dir_to(odx);
		const Vec3 tody = t.dir_to(ody);
		const Vec3 tddx = t.dir_to(ddx);
		const Vec3 tddy = t.dir_to(ddy);

		// Translate differentials into ray width approximation
		// TODO: do this correctly for arbitrary ray differentials,
		// not just camera ray differentials.
		r.ow = std::min(todx.length(), tody.length());
		r.dw = std::min(tddx.length(), tddy.length());

		// Finalize ray
		r.finalize();
	}

	/**
	 * Creates a Ray from the WorldRay.
	 */
	Ray to_ray() const {
		Ray r;

		r.time = time;

		// Ray type
		if (type == OCCLUSION) {
			r.max_t = 1.0f;
			r.flags() |= Ray::IS_OCCLUSION;
		}

		update_ray(&r);
		return r;
	}

	Ray to_ray(const Transform& t) const {
		Ray r;

		r.time = time;

		// Ray type
		if (type == OCCLUSION) {
			r.max_t = 1.0f;
			r.flags() |= Ray::IS_OCCLUSION;
		}

		update_ray(&r, t);
		return r;
	}

};



#endif
