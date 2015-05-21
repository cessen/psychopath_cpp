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


/**
 * @brief A ray in 3d space.
 */

struct Ray {
	// Ray data
	// Weird interleaving of fields is for alignment
	Vec3 o; // Origin
	float max_t; // Maximum extent along the ray
	Vec3 d; // Direction
	float time; // Time coordinate
	Vec3 d_inv; // 1.0 / d
	float owx, owy; // Origin width
	float dwx, dwy; // Width delta
	float fwx, fwy; // Width floor
	uint32_t id_and_flags;  // Ray id and flags, packed into a single int.
	BitStack<uint64_t> trav_stack;  // Bit stack used during BVH traversal


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
	  time {time_},
	  id_and_flags {0}
	{}

	// Access to occlusion flag
	bool is_occlusion() const {
		return id_and_flags & (1 << 30);
	}

	void set_occlusion_true() {
		id_and_flags |= (1 << 30);
	}

	void set_occlusion_false() {
		id_and_flags &= ~(1 << 30);
	}

	// Access to done flag
	bool is_done() const {
		return id_and_flags & (1 << 31);
	}

	void set_done_true() {
		id_and_flags |= (1 << 31);
	}

	void set_done_false() {
		id_and_flags &= ~(1 << 31);
	}

	// Access to ray id
	uint32_t id() const {
		return id_and_flags & ((~uint32_t {0}) >> 2);
	}

	void set_id(uint32_t n) {
		id_and_flags &= ~((~uint32_t {0}) >> 2);
		id_and_flags |= n & ((~uint32_t {0}) >> 2);
	}

	// Access to inverse direction
	Vec3 get_d_inverse() const {
		return d_inv;
	}

	/**
	 * Computes the acceleration data for speedy bbox intersection testing.
	 */
	void update_accel() {
		// Inverse direction
		d_inv = Vec3(1.0f, 1.0f, 1.0f) / d;
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
		const float x = std::abs((owx - fwx) + (dwx * t)) + fwx;
		const float y = std::abs((owy - fwy) + (dwy * t)) + fwy;
		return std::min(x, y);
	}

	/*
	 * Returns an estimate of the minimum ray width over a distance
	 * range along the ray.
	 */
	float min_width(const float tnear, const float tfar) const {
		//return std::min(width(tnear), width(tfar));

		const float tflipx = (owx - fwx) / dwx;
		const float tflipy = (owy - fwy) / dwy;

		float minx, miny;

		if (tnear < tflipx && tfar > tflipx) {
			minx = fwx;
		} else {
			minx = std::min(std::abs((owx - fwx) + (dwx * tnear)) + fwx, std::abs((owx - fwx) + (dwx * tfar)) + fwx);
		}

		if (tnear < tflipy && tfar > tflipy) {
			miny = fwy;
		} else {
			miny = std::min(std::abs((owy - fwy) + (dwy * tnear)) + fwy, std::abs((owy - fwy) + (dwy * tfar)) + fwy);
		}

		return std::min(minx, miny);
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

	/**
	 * Modifies a Ray in-place to be consistent with the WorldRay.
	 */
	void update_ray(Ray* ray) const {
		Ray& r = *ray;

		// Origin, direction, and time
		r.o = o;
		r.d = d;

		// Convert differentials into ray width approximation

		// X ray differential turned into a ray
		const Vec3 orx = r.o + odx;
		const Vec3 drx = r.d + ddx;

		// Y ray differential turned into a ray
		const Vec3 ory = r.o + ody;
		const Vec3 dry = r.d + ddy;

		// Find t where dx and dy are smallest, respectively.
		float tdx, lx;
		float tdy, ly;
		std::tie(tdx, lx) = closest_ray_t2(r.o, r.d, orx, drx);
		std::tie(tdy, ly) = closest_ray_t2(r.o, r.d, ory, dry);

		// Set x widths
		r.owx = odx.length();
		if (tdx <= 0.0f) {
			r.dwx = ddx.length();
			r.fwx = 0.0f;
		} else {
			r.dwx = (lx - r.owx) / tdx;
			r.fwx = lx;
		}

		// Set y widths
		r.owy = ody.length();
		if (tdy <= 0.0f) {
			r.dwy = ddy.length();
			r.fwy = 0.0f;
		} else {
			r.dwy = (ly - r.owy) / tdy;
			r.fwy = ly;
		}

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

		// Convert differentials into ray width approximation

		// X ray differential turned into a ray
		const Vec3 orx = r.o + todx;
		const Vec3 drx = r.d + tddx;

		// Y ray differential turned into a ray
		const Vec3 ory = r.o + tody;
		const Vec3 dry = r.d + tddy;

		// Find t where dx and dy are smallest, respectively.
		float tdx, lx;
		float tdy, ly;
		std::tie(tdx, lx) = closest_ray_t2(r.o, r.d, orx, drx);
		std::tie(tdy, ly) = closest_ray_t2(r.o, r.d, ory, dry);

		// Set x widths
		r.owx = todx.length();
		if (tdx <= 0.0f) {
			r.dwx = tddx.length();
			r.fwx = 0.0f;
		} else {
			r.dwx = (lx - r.owx) / tdx;
			r.fwx = lx;
		}

		// Set y widths
		r.owy = tody.length();
		if (tdy <= 0.0f) {
			r.dwy = tddy.length();
			r.fwy = 0.0f;
		} else {
			r.dwy = (ly - r.owy) / tdy;
			r.fwy = ly;
		}

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
			r.set_occlusion_true();
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
			r.set_occlusion_true();
		}

		update_ray(&r, t);
		return r;
	}

};



#endif
