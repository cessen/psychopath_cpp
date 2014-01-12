#ifndef BBOX_HPP
#define BBOX_HPP

#include "numtype.h"

#include <limits>
#include <iostream>
#include <cmath>
#include <stdlib.h>
#include <tuple>
#include <x86intrin.h>
#include "global.hpp"
#include "timebox.hpp"
#include "vector.hpp"
#include "ray.hpp"
#include "utils.hpp"


/**
 * @brief An axis-aligned bounding box.
 */
struct BBox {
	// Default is a degenerate BBox with min as Inf and max as -Inf.
	Vec3 min {std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity()};
	Vec3 max {-std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity()};

	/**
	 * @brief Default constructor.
	 */
	BBox() {}

	BBox(const BBox& b): min {b.min}, max {b.max}
	{}

	BBox& operator=(const BBox& b) {
		min = b.min;
		max = b.max;

		return *this;
	}

	/**
	 * Initializing constructor.
	 */
	BBox(const Vec3& min_, const Vec3& max_): min {min_}, max {max_} {}

	/**
	 * @brief Adds two BBox's together in a component-wise manner.
	 */
	BBox operator+(const BBox& b) const {
		return BBox(min + b.min, max + b.max);
	}

	/**
	 * @brief Subtracts one BBox from another in a component-wise manner.
	 */
	BBox operator-(const BBox& b) const {
		return BBox(min - b.min, max - b.max);
	}

	/**
	 * @brief Multiples all the components of a BBox by a float.
	 */
	BBox operator*(const float& f) const {
		return BBox(min * f, max * f);
	}

	/**
	 * @brief Divides all the components of a BBox by a float.
	 */
	BBox operator/(const float& f) const {
		return BBox(min / f, max / f);
	}

	/**
	 * @brief Union of two BBoxes.
	 */
	BBox operator|(const BBox& b) {
		BBox temp;
		for (size_t i = 0; i < 3; i++) {
			temp.min[i] = min[i] < b.min[i] ? min[i] : b.min[i];
			temp.max[i] = max[i] > b.max[i] ? max[i] : b.max[i];
		}
		return temp;
	}

	/**
	 * @brief Intersection of two BBoxes.
	 */
	BBox operator&(const BBox& b) {
		BBox temp;
		for (size_t i = 0; i < 3; i++) {
			temp.min[i] = min[i] > b.min[i] ? min[i] : b.min[i];
			temp.max[i] = max[i] < b.max[i] ? max[i] : b.max[i];
		}
		return temp;
	}

	/**
	 * @brief Merge another BBox into this one.
	 *
	 * Merges another BBox into this one, resulting in a BBox that fully
	 * encompasses both.
	 */
	void merge_with(const BBox& b) {
		for (size_t i = 0; i < 3; i++) {
			min[i] = min[i] < b.min[i] ? min[i] : b.min[i];
			max[i] = max[i] > b.max[i] ? max[i] : b.max[i];
		}
	}

	/**
	 * @brief Tests a ray against the BBox.
	 *
	 * @param[in] ray The ray to test against.
	 * @param[out] hitt0 If there is a hit, this gets modified to the
	 *     distance to the closest intersection.
	 * @param[out] hitt1 If there is a hit, this gets modified to the
	 *     distance to the furthest intersection.
	 * @param[in] an optional alternative ray max_t to use.
	 *
	 * @returns True if the ray hits, false if the ray misses.
	 */
	inline bool intersect_ray(const Ray& ray, const Vec3 inv_d, const std::array<uint32_t, 3> d_is_neg, float *hitt0, float *hitt1, float *t=nullptr) const {
#ifdef DEBUG
		// Test for nan and inf
		if (std::isnan(ray.o.x) || std::isnan(ray.o.y) || std::isnan(ray.o.z) ||
		        std::isnan(ray.d.x) || std::isnan(ray.d.y) || std::isnan(ray.d.z) ||
		        std::isnan(inv_d.x) || std::isnan(inv_d.y) || std::isnan(inv_d.z) ||
		        std::isnan(min.x) || std::isnan(min.y) || std::isnan(min.z) ||
		        std::isnan(max.x) || std::isnan(max.y) || std::isnan(max.z)) {
			std::cout << "NaN found!" << std::endl;
			Global::Stats::nan_count++;
		}

		if (std::isinf(ray.o.x) || std::isinf(ray.o.y) || std::isinf(ray.o.z) ||
		        std::isinf(ray.d.x) || std::isinf(ray.d.y) || std::isinf(ray.d.z) ||
		        std::isinf(min.x) || std::isinf(min.y) || std::isinf(min.z) ||
		        std::isinf(max.x) || std::isinf(max.y) || std::isinf(max.z)) {
			std::cout << "Inf found!" << std::endl;
			Global::Stats::inf_count++;
		}
#endif

		const Vec3 *bounds = &min;

		float tmin = (bounds[d_is_neg[0]].x - ray.o.x) * inv_d.x;
		float tmax = (bounds[1-d_is_neg[0]].x - ray.o.x) * inv_d.x;
		const float tymin = (bounds[d_is_neg[1]].y - ray.o.y) * inv_d.y;
		const float tymax = (bounds[1-d_is_neg[1]].y - ray.o.y) * inv_d.y;
		const float tzmin = (bounds[d_is_neg[2]].z - ray.o.z) * inv_d.z;
		const float tzmax = (bounds[1-d_is_neg[2]].z - ray.o.z) * inv_d.z;

		if (tymin > tmin)
			tmin = tymin;
		if (tzmin > tmin)
			tmin = tzmin;
		if (tymax < tmax)
			tmax = tymax;
		if (tzmax < tmax)
			tmax = tzmax;

		const float tt = (t != nullptr) ? *t : ray.max_t;
		if ((tmin <= tmax) && (tmin < tt) && (tmax > 0.0f)) {
			*hitt0 = tmin > 0.0f ? tmin : 0.0f;
			*hitt1 = tmax < tt ? tmax : tt;
			return true;
		} else {
			return false;
		}
	}

	inline bool intersect_ray(const Ray& ray, float *hitt0, float *hitt1, float *t=nullptr) const {
		const Vec3 inv_d = ray.get_inverse_d();
		const std::array<uint32_t, 3> d_is_neg = ray.get_d_is_neg();

		return intersect_ray(ray, inv_d, d_is_neg, hitt0, hitt1, t);
	}


	/**
	 * @brief Tests a ray against the BBox.
	 *
	 * @param ray The ray to test against.
	 *
	 * @returns True if the ray hits, false if the ray misses.
	 */
	inline bool intersect_ray(const Ray& ray, const Vec3 inv_d, const std::array<uint32_t, 3> d_is_neg) const {
		float hitt0, hitt1;
		return intersect_ray(ray, inv_d, d_is_neg, &hitt0, &hitt1);
	}

	inline bool intersect_ray(const Ray& ray) const {
		float hitt0, hitt1;
		return intersect_ray(ray, &hitt0, &hitt1);
	}

	/**
	 * @brief Returns the surface area of the BBox.
	 */
	float surface_area() const {
		const float x = max.x - min.x;
		const float y = max.y - min.y;
		const float z = max.z - min.z;

		return 2 * (x*y + x*z + y*z);
	}

	/**
	 * @brief Returns the length of the diagonal of the BBox.
	 */
	float diagonal() const {
		return (max - min).length();
	}

	/**
	 * @brief Returns the center point of the BBox.
	 */
	Vec3 center() const {
		return (min + max) * 0.5f;
	}

	std::string to_string() const {
		std::string s;
		s.append("(");
		s.append(std::to_string(min[0]));
		s.append(", ");
		s.append(std::to_string(min[1]));
		s.append(", ");
		s.append(std::to_string(min[2]));
		s.append(") (");
		s.append(std::to_string(max[0]));
		s.append(", ");
		s.append(std::to_string(max[1]));
		s.append(", ");
		s.append(std::to_string(max[2]));
		s.append(")");
		return s;
	}
};


struct alignas(16) BBox2 {
    union {
        __m128 bounds[3]; // Layed out as xmin1, xmin2 ,xmax1, xmax2, ymin1, ymin2, etc
        // Each __m128 contains all the x's, y's, or z's for both bounding boxes
float bounds_f[12] = {
	std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(),
	-std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(),
	std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(),
	-std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(),
	std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(),
	-std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity()
};
    };

BBox2() {}

// Construct from two BBox's
BBox2(const BBox& b1, const BBox& b2) {
	bounds_f[0]  = b1.min.x;
	bounds_f[1]  = b2.min.x;
	bounds_f[2]  = b1.max.x;
	bounds_f[3]  = b2.max.x;
	bounds_f[4]  = b1.min.y;
	bounds_f[5]  = b2.min.y;
	bounds_f[6]  = b1.max.y;
	bounds_f[7]  = b2.max.y;
	bounds_f[8]  = b1.min.z;
	bounds_f[9]  = b2.min.z;
	bounds_f[10] = b1.max.z;
	bounds_f[11] = b2.max.z;
}

// Basic operators
BBox2& operator=(const BBox2& b) {
	for (int i = 0; i < 12; ++i)
		bounds_f[i] = b.bounds_f[i];

	return *this;
}

BBox2 operator+(const BBox2& b) const {
	BBox2 result;
	for (int i = 0; i < 12; ++i)
		result.bounds_f[i] = bounds_f[i] + b.bounds_f[i];
	return result;
}

BBox2 operator-(const BBox2& b) const {
	BBox2 result;
	for (int i = 0; i < 12; ++i)
		result.bounds_f[i] = bounds_f[i] - b.bounds_f[i];
	return result;
}

BBox2 operator*(const BBox2& b) const {
	BBox2 result;
	for (int i = 0; i < 12; ++i)
		result.bounds_f[i] = bounds_f[i] * b.bounds_f[i];
	return result;
}

BBox2 operator*(const float f) const {
	BBox2 result;
	for (int i = 0; i < 12; ++i)
		result.bounds_f[i] = bounds_f[i] * f;
	return result;
}

BBox2 operator/(const BBox2& b) const {
	BBox2 result;
	for (int i = 0; i < 12; ++i)
		result.bounds_f[i] = bounds_f[i] / b.bounds_f[i];
	return result;
}

BBox2 operator/(const float f) const {
	BBox2 result;
	for (int i = 0; i < 12; ++i)
		result.bounds_f[i] = bounds_f[i] / f;
	return result;
}

// Union
BBox2 operator|(const BBox2& b) {
	BBox2 result;
	for (int i = 0; i < 3; ++i) {
		const int i1 = i * 4;
		const int i2 = (i * 4) + 1;
		const int i3 = (i * 4) + 2;
		const int i4 = (i * 4) + 3;
		result.bounds_f[i1] = std::min(bounds_f[i1], b.bounds_f[i1]);
		result.bounds_f[i2] = std::min(bounds_f[i2], b.bounds_f[i2]);
		result.bounds_f[i3] = std::max(bounds_f[i3], b.bounds_f[i3]);
		result.bounds_f[i4] = std::max(bounds_f[i4], b.bounds_f[i4]);
	}
	return result;
}

// Intersection
BBox2 operator&(const BBox2& b) {
	BBox2 result;
	for (int i = 0; i < 3; ++i) {
		const int i1 = i * 4;
		const int i2 = (i * 4) + 1;
		const int i3 = (i * 4) + 2;
		const int i4 = (i * 4) + 3;
		result.bounds_f[i1] = std::max(bounds_f[i1], b.bounds_f[i1]);
		result.bounds_f[i2] = std::max(bounds_f[i2], b.bounds_f[i2]);
		result.bounds_f[i3] = std::min(bounds_f[i3], b.bounds_f[i3]);
		result.bounds_f[i4] = std::min(bounds_f[i4], b.bounds_f[i4]);
	}

	return result;
}

/**
 * @brief Merge another BBox into this one.
 *
 * Merges another BBox into this one, resulting in a BBox that fully
 * encompasses both.
 */
BBox2& merge_with(const BBox2& b) {
	for (int i = 0; i < 3; ++i) {
		const int i1 = i * 4;
		const int i2 = (i * 4) + 1;
		const int i3 = (i * 4) + 2;
		const int i4 = (i * 4) + 3;
		bounds_f[i1] = std::min(bounds_f[i1], b.bounds_f[i1]);
		bounds_f[i2] = std::min(bounds_f[i2], b.bounds_f[i2]);
		bounds_f[i3] = std::max(bounds_f[i3], b.bounds_f[i3]);
		bounds_f[i4] = std::max(bounds_f[i4], b.bounds_f[i4]);
	}

	return *this;
}

/**
 * @brief Tests a ray against the BBox2's bounding boxes.
 *
 * @param[in] o The origin of the ray, laid out as [[x,x,x,x],[y,y,y,y],[z,z,z,z]].
 * @param[in] inv_d The direction of the ray over 1.0, laid out as [[x,x,x,x],[y,y,y,y],[z,z,z,z]]
 * @param[in] t_max The maximum t value of the ray being tested against.
 * @param[in] d_is_neg Precomputed values indicating whether the x, y, and z components of the ray are negative or not.
 * @param[out] near_hits Two floats to store the near hits with the bounding boxes.
 *
 * @returns A tuple of two boolean values, indicating whether the ray hit the first and second box respectively.
 */
inline std::tuple<bool, bool> intersect_ray(const __m128* o, const __m128* inv_d, float t_max, const std::array<uint32_t, 3> d_is_neg, float *near_hits) const {
	// Calculate the plane intersections
	__m128 inters[3] = {_mm_mul_ps(_mm_sub_ps(bounds[0], o[0]), inv_d[0]),
	                    _mm_mul_ps(_mm_sub_ps(bounds[1], o[1]), inv_d[1]),
	                    _mm_mul_ps(_mm_sub_ps(bounds[2], o[2]), inv_d[2])
	                   };

	// Swap min/max values depending on ray direction
	const unsigned int shuf_min_max = (1<<6) | (0<<4) | (3<<2) | 2; // Swaps the upper and lower floats
	for (int i = 0; i < 3; ++i) {
		if (d_is_neg[i])
			inters[i] = _mm_shuffle_ps(inters[i], inters[i], shuf_min_max);
	}

	// Get the minimum and maximum hits, and shuffle the max hits
	// to be in the same location as the minimum hits
	const __m128 mins      = _mm_max_ps(_mm_max_ps(inters[0], inters[1]), inters[2]);
	const __m128 temp_maxs = _mm_min_ps(_mm_min_ps(inters[0], inters[1]), inters[2]);
	const __m128 maxs =  _mm_shuffle_ps(temp_maxs, temp_maxs, shuf_min_max);

	// Get results
	//float min_f[4];
	//float max_f[4];
	//_mm_store_ps(min_f, mins);
	//_mm_store_ps(max_f, maxs);

	// Check for hits
	bool hit0 = false;
	bool hit1 = false;
	if (maxs[0] > 0.0f && mins[0] < t_max && mins[0] <= maxs[0]) {
		hit0 = true;
		near_hits[0] = std::max(mins[0], 0.0f);
	}
	if (maxs[1] > 0.0f && mins[1] < t_max && mins[1] <= maxs[1]) {
		hit1 = true;
		near_hits[1] = std::max(mins[1], 0.0f);
	}

	return std::make_tuple(hit0, hit1);
}


inline std::tuple<bool, bool> intersect_ray(const Ray& ray, float *near_hits) const {
	const Vec3 inv_d_f = ray.get_inverse_d();
	const std::array<uint32_t, 3> d_is_neg = ray.get_d_is_neg();

	// Load ray origin and inverse direction into simd layouts for intersection testing
	__m128 ray_o[3];
	__m128 inv_d[3];
	for (int i = 0; i < 3; ++i) {
		ray_o[i] = _mm_load_ps1(&(ray.o[i]));
		inv_d[i] = _mm_load_ps1(&(inv_d_f[i]));
	}

	return intersect_ray(ray_o, inv_d, ray.max_t, d_is_neg, near_hits);
}

};


/**
 * @brief Axis-aligned bounding box with multiple time samples.
 */
struct BBoxT {
public:
	TimeBox<BBox> bbox;

	BBoxT(const int32_t &res_time=1);
	BBoxT(const Vec3 &bmin_, const Vec3 &bmax_);

	bool init(const uint8_t &state_count_) {
		return bbox.init(state_count_);
	}

	void add_time_sample(const int32_t &samp, const Vec3 &bmin_, const Vec3 &bmax_) {
		bbox[samp].min = bmin_;
		bbox[samp].max = bmax_;
	}

	/**
	 * @brief Fetches the BBox at time t.
	 */
	BBox at_time(const float t) {
		int32_t ia=0, ib=0;
		float alpha=0.0;
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
	size_t size() {
		return bbox.size();
	}

	BBox &operator[](const int32_t &i) {
		return bbox.states[i];
	}

	const BBox &operator[](const int32_t &i) const {
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
	float surface_area() const {
		return bbox[0].surface_area();
	}

	/**
	 * @brief Intersects a ray with the BBoxT.
	 *
	 * @param[out] hitt0 Near hit is placed here if there is a hit.
	 * @param[out] hitt1 Far hit is placed here if there is a hit.
	 */
	inline bool intersect_ray(const Ray &ray, float *hitt0, float *hitt1) {
		return at_time(ray.time).intersect_ray(ray, hitt0, hitt1);
	}

	/**
	 * @brief Intersects a ray with the BBoxT.
	 */
	inline bool intersect_ray(const Ray &ray) {
		float hitt0, hitt1;
		return at_time(ray.time).intersect_ray(ray, &hitt0, &hitt1);
	}
};

#endif // BBOX_HPP

