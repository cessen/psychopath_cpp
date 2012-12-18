#ifndef BBOX_HPP
#define BBOX_HPP

#include "numtype.h"

#include <stdlib.h>
#include <x86intrin.h>
#include "timebox.hpp"
#include "vector.hpp"
#include "ray.hpp"
#include "utils.hpp"


/**
 * @brief An axis-aligned bounding box.
 */
struct BBox {
	Vec3 min, max;

	BBox() {
		min.x = 0.0;
		min.y = 0.0;
		min.z = 0.0;

		max.x = 0.0;
		max.y = 0.0;
		max.z = 0.0;
	}

	BBox(const Vec3 &min_, const Vec3 &max_) {
		min = min_;
		max = max_;
	}

	/**
	 * @brief Adds two BBox's together in a component-wise manner.
	 */
	BBox operator+(const BBox &b) const {
		BBox c;

		c.min = min + b.min;
		c.max = max + b.max;

		return c;
	}

	/**
	 * @brief Subtracts one BBox from another in a component-wise manner.
	 */
	BBox operator-(const BBox &b) const {
		BBox c;

		c.min = min - b.min;
		c.max = max - b.max;

		return c;
	}

	/**
	 * @brief Multiples all the components of a BBox by a float.
	 */
	BBox operator*(const float32 &f) const {
		BBox c;

		c.min = min * f;
		c.max = max * f;

		return c;
	}

	/**
	 * @brief Divides all the components of a BBox by a float.
	 */
	BBox operator/(const float32 &f) const {
		BBox c;

		c.min = min / f;
		c.max = max / f;

		return c;
	}

	/**
	 * @brief Merge another BBox into this one.
	 *
	 * Merges another BBox into this one, resulting in a BBox that fully
	 * encompasses both.
	 */
	void merge_with(const BBox &b) {
#if 0
		min.m128 = _mm_min_ps(min.m128, b.min.m128);
		max.m128 = _mm_max_ps(max.m128, b.max.m128);
#else
		for (uint_i i = 0; i < 3; i++) {
			min[i] = min[i] < b.min[i] ? min[i] : b.min[i];
			max[i] = max[i] > b.max[i] ? max[i] : b.max[i];
		}
#endif
	}

	/**
	 * @brief Tests a ray against the BBox.
	 *
	 * @param[in] ray The ray to test against.
	 * @param[out] hitt0 If there is a hit, this gets modified to the
	 *     distance to the closest intersection.
	 * @param[out] hitt1 If there is a hit, this gets modified to the
	 *     distance to the furthest intersection.
	 *
	 * @returns True if the ray hits, false if the ray misses.
	 */
	inline bool intersect_ray(const Ray &ray, float32 *hitt0, float32 *hitt1) const {
		const Vec3 *bounds = &min;

		float32 tmin = (bounds[ray.d_is_neg[0]].x - ray.o.x) * ray.inv_d.x;
		float32 tmax = (bounds[1-ray.d_is_neg[0]].x - ray.o.x) * ray.inv_d.x;
		const float32 tymin = (bounds[ray.d_is_neg[1]].y - ray.o.y) * ray.inv_d.y;
		const float32 tymax = (bounds[1-ray.d_is_neg[1]].y - ray.o.y) * ray.inv_d.y;
		const float32 tzmin = (bounds[ray.d_is_neg[2]].z - ray.o.z) * ray.inv_d.z;
		const float32 tzmax = (bounds[1-ray.d_is_neg[2]].z - ray.o.z) * ray.inv_d.z;

		if (tymin > tmin)
			tmin = tymin;
		if (tzmin > tmin)
			tmin = tzmin;
		if (tymax < tmax)
			tmax = tymax;
		if (tzmax < tmax)
			tmax = tzmax;

		if ((tmin < tmax) && (tmin < ray.max_t) && (tmax > ray.min_t)) {
			*hitt0 = tmin > ray.min_t ? tmin : ray.min_t;
			*hitt1 = tmax < ray.max_t ? tmax : ray.max_t;
			return true;
		} else {
			return false;
		}
	}


	/**
	 * @brief Tests a ray against the BBox.
	 *
	 * @param ray The ray to test against.
	 *
	 * @returns True if the ray hits, false if the ray misses.
	 */
	inline bool intersect_ray(const Ray &ray) const {
		float32 hitt0, hitt1;
		return intersect_ray(ray, &hitt0, &hitt1);
	}

	/**
	 * @brief Returns the surface area of the BBox.
	 */
	float32 surface_area() const {
		const float32 x = max.x - min.x;
		const float32 y = max.y - min.y;
		const float32 z = max.z - min.z;

		return 2 * (x*y + x*z + y*z);
	}
};




/**
 * @brief Axis-aligned bounding box with multiple time samples.
 *
 * A BBox that can include multiple time samples.  This is the version of
 * BBox that should be used in most places throughout the code.
 */
struct BBoxT {
public:
	TimeBox<BBox> bbox;

	BBoxT(const int32 &res_time=1);
	BBoxT(const Vec3 &bmin_, const Vec3 &bmax_);

	bool init(const uint8 &state_count_) {
		return bbox.init(state_count_);
	}

	void add_time_sample(const int32 &samp, const Vec3 &bmin_, const Vec3 &bmax_) {
		bbox[samp].min = bmin_;
		bbox[samp].max = bmax_;
	}

	/**
	 * @brief Fetches the BBox at time t.
	 */
	BBox at_time(const float32 t) {
		int32 ia=0, ib=0;
		float32 alpha=0.0;
		bool motion;

		motion = bbox.query_time(t, &ia, &ib, &alpha);

		if (motion) {
			return lerp(alpha, bbox[ia], bbox[ib]);
		} else {
			return bbox[0];
		}
	}

	/**
	 * @brief Number of time samples.
	 */
	uint_i size() {
		return bbox.size();
	}

	BBox &operator[](const int32 &i) {
		return bbox.states[i];
	}

	const BBox &operator[](const int32 &i) const {
		return bbox.states[i];
	}

	/**
	 * @brief Copies another BBox into this one.
	 *
	 * Overwrites any bounds that were already there.
	 */
	void copy(const BBoxT &b);

	/**
	 * @brief Merges another BBox into this one.
	 *
	 * Results in a new minimal BBox that contains both the originals.
	 */
	void merge_with(const BBoxT &b) {
		// BBoxes have the same state count, so we
		// can just merge each corresponding state.
		if (bbox.state_count == b.bbox.state_count) {
			for (int i=0; i < bbox.state_count; i++) {
				bbox[i].merge_with(b.bbox[i]);
			}
		}
		// BBoxes have differing state count, so we
		// merge into a single-state bbox.
		// TODO: something more sophisticated.
		else {
			BBox bb = bbox[0];
			for (int i=1; i < bbox.state_count; i++)
				bb.merge_with(bbox[i]);
			for (int i=0; i < b.bbox.state_count; i++)
				bb.merge_with(b.bbox[i]);
			init(1);
			bbox[0] = bb;
		}
	}

	/**
	 * @brief Returns the surface area of the BBoxT.
	 * For now just takes the first time sample.
	 */
	float32 surface_area() const {
		return bbox[0].surface_area();
	}

	/**
	 * @brief Intersects a ray with the BBoxT.
	 *
	 * @param[out] hitt0 Near hit is placed here if there is a hit.
	 * @param[out] hitt1 Far hit is placed here if there is a hit.
	 */
	inline bool intersect_ray(const Ray &ray, float32 *hitt0, float32 *hitt1) {
		return at_time(ray.time).intersect_ray(ray, hitt0, hitt1);
	}

	/**
	 * @brief Intersects a ray with the BBoxT.
	 */
	inline bool intersect_ray(const Ray &ray) {
		float32 hitt0, hitt1;
		return at_time(ray.time).intersect_ray(ray, &hitt0, &hitt1);
	}
};

#endif // BBOX_HPP

