#ifndef BBOX_HPP
#define BBOX_HPP

#include "numtype.h"

#include <limits>
#include <iostream>
#include <cmath>
#include <stdlib.h>
#include <tuple>
#include "simd.hpp"
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
	inline bool intersect_ray(const Ray& ray, const Vec3 d_inv, const std::array<uint32_t, 3> d_sign, float *hitt0, float *hitt1, float *t=nullptr) const {
#ifdef DEBUG
		// Test for nan and inf
		if (std::isnan(ray.o.x) || std::isnan(ray.o.y) || std::isnan(ray.o.z) ||
		        std::isnan(ray.d.x) || std::isnan(ray.d.y) || std::isnan(ray.d.z) ||
		        std::isnan(d_inv.x) || std::isnan(d_inv.y) || std::isnan(d_inv.z) ||
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

		float tmin = (bounds[d_sign[0]].x - ray.o.x) * d_inv.x;
		float tmax = (bounds[1-d_sign[0]].x - ray.o.x) * d_inv.x;
		const float tymin = (bounds[d_sign[1]].y - ray.o.y) * d_inv.y;
		const float tymax = (bounds[1-d_sign[1]].y - ray.o.y) * d_inv.y;
		const float tzmin = (bounds[d_sign[2]].z - ray.o.z) * d_inv.z;
		const float tzmax = (bounds[1-d_sign[2]].z - ray.o.z) * d_inv.z;

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
		const Vec3 d_inv = ray.get_d_inverse();
		const std::array<uint32_t, 3> d_sign = ray.get_d_sign();

		return intersect_ray(ray, d_inv, d_sign, hitt0, hitt1, t);
	}


	/**
	 * @brief Tests a ray against the BBox.
	 *
	 * @param ray The ray to test against.
	 *
	 * @returns True if the ray hits, false if the ray misses.
	 */
	inline bool intersect_ray(const Ray& ray, const Vec3 d_inv, const std::array<uint32_t, 3> d_sign) const {
		float hitt0, hitt1;
		return intersect_ray(ray, d_inv, d_sign, &hitt0, &hitt1);
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


struct BBox2 {
	SIMD::float4 bounds[3];

	BBox2(): bounds {{std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity()},
		{std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity()},
		{std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity()}
	}
	{}

	// Construct from two BBox's
	BBox2(const BBox& b1, const BBox& b2): bounds {{b1.min.x, b2.min.x, b1.max.x, b2.max.x},
		{b1.min.y, b2.min.y, b1.max.y, b2.max.y},
		{b1.min.z, b2.min.z, b1.max.z, b2.max.z}
	}
	{}

	BBox2 operator+(const BBox2& b) const {
		BBox2 result;
		for (int i = 0; i < 3; ++i)
			result.bounds[i] = bounds[i] + b.bounds[i];
		return result;
	}

	BBox2 operator-(const BBox2& b) const {
		BBox2 result;
		for (int i = 0; i < 3; ++i)
			result.bounds[i] = bounds[i] - b.bounds[i];
		return result;
	}

	BBox2 operator*(const BBox2& b) const {
		BBox2 result;
		for (int i = 0; i < 3; ++i)
			result.bounds[i] = bounds[i] * b.bounds[i];
		return result;
	}

	BBox2 operator*(const float f) const {
		BBox2 result;
		for (int i = 0; i < 3; ++i)
			result.bounds[i] = bounds[i] * f;
		return result;
	}

	BBox2 operator/(const BBox2& b) const {
		BBox2 result;
		for (int i = 0; i < 3; ++i)
			result.bounds[i] = bounds[i] / b.bounds[i];
		return result;
	}

	BBox2 operator/(const float f) const {
		BBox2 result;
		for (int i = 0; i < 3; ++i)
			result.bounds[i] = bounds[i] / f;
		return result;
	}

	// Union
	BBox2 operator|(const BBox2& b) {
		BBox2 result;
		for (int i = 0; i < 3; ++i) {
			result.bounds[i][0] = std::min(bounds[i][0], b.bounds[i][0]);
			result.bounds[i][1] = std::min(bounds[i][1], b.bounds[i][1]);
			result.bounds[i][2] = std::max(bounds[i][2], b.bounds[i][2]);
			result.bounds[i][3] = std::max(bounds[i][3], b.bounds[i][3]);
		}
		return result;
	}

	// Intersection
	BBox2 operator&(const BBox2& b) {
		BBox2 result;
		for (int i = 0; i < 3; ++i) {
			result.bounds[i][0] = std::max(bounds[i][0], b.bounds[i][0]);
			result.bounds[i][1] = std::max(bounds[i][1], b.bounds[i][1]);
			result.bounds[i][2] = std::min(bounds[i][2], b.bounds[i][2]);
			result.bounds[i][3] = std::min(bounds[i][3], b.bounds[i][3]);
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
			bounds[i][0] = std::min(bounds[i][0], b.bounds[i][0]);
			bounds[i][1] = std::min(bounds[i][1], b.bounds[i][1]);
			bounds[i][2] = std::max(bounds[i][2], b.bounds[i][2]);
			bounds[i][3] = std::max(bounds[i][3], b.bounds[i][3]);
		}
		return *this;
	}

	/**
	 * @brief Tests a ray against the BBox2's bounding boxes.
	 *
	 * @param[in] o The origin of the ray, laid out as [[x,x,x,x],[y,y,y,y],[z,z,z,z]].
	 * @param[in] d_inv The direction of the ray over 1.0, laid out as [[x,x,x,x],[y,y,y,y],[z,z,z,z]]
	 * @param[in] t_max The maximum t value of the ray being tested against, laid out as [t,t,t,t].
	 * @param[in] d_sign Precomputed values indicating whether the x, y, and z components of the ray are negative or not.
	 * @param[out] hit_ts Pointer to a SIMD::float4 where the t parameter of each hit (if any) will be recorded.
	 *             The hit t's are stored in index [0] and [1] for the first and second box, respectively.
	 *
	 * @returns A bitmask indicating which (if any) of the two boxes were hit.
	 */
	inline unsigned int intersect_ray(const SIMD::float4* o, const SIMD::float4* d_inv, const SIMD::float4& t_max, const std::array<uint32_t, 3>& d_sign, SIMD::float4 *hit_ts) const {
		using namespace SIMD;
		const float4 zeros(0.0f);

		// Calculate the plane intersections
		const float4 xs = (shuffle_swap(bounds[0], d_sign[0]) - o[0]) * d_inv[0];
		const float4 ys = (shuffle_swap(bounds[1], d_sign[1]) - o[1]) * d_inv[1];
		const float4 zs = (shuffle_swap(bounds[2], d_sign[2]) - o[2]) * d_inv[2];

		// Get the minimum and maximum hits, and shuffle the max hits
		// to be in the same location as the minimum hits
		const float4 mins = max(max(xs, ys), max(zs, zeros));
		const float4 maxs = shuffle_swap(min(min(xs, ys), zs));

		// Check for hits
		const float4 hits = lt(mins, t_max) && lte(mins, maxs);

		// Fill in near hits
		*hit_ts = mins;

		return to_bitmask(hits) & 3;
	}


	inline unsigned int intersect_ray(const Ray& ray, SIMD::float4 *hit_ts) const {
		using namespace SIMD;
		const Vec3 d_inv_f = ray.get_d_inverse();
		const std::array<uint32_t, 3> d_sign = ray.get_d_sign();

		// Load ray origin, inverse direction, and max_t into simd layouts for intersection testing
		const float4 ray_o[3] = {ray.o[0], ray.o[1], ray.o[2]};
		const float4 d_inv[3] = {d_inv_f[0], d_inv_f[1], d_inv_f[2]};
		const float4 max_t {
			ray.max_t
		};

		return intersect_ray(ray_o, d_inv, max_t, d_sign, hit_ts);
	}

};


struct BBox4 {
	SIMD::float4 bounds[6];

	BBox4(): bounds {
		{std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity()},
		{-std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity()},
		{std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity()},
		{-std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity()},
		{std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity()},
		{-std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity()}
	}
	{}

	// Construct from two BBox's
	BBox4(const BBox& b1, const BBox& b2, const BBox& b3, const BBox& b4): bounds {
		{b1.min.x, b2.min.x, b3.min.x, b4.min.x},
		{b1.max.x, b2.max.x, b3.max.x, b4.max.x},
		{b1.min.y, b2.min.y, b3.min.y, b4.min.y},
		{b1.max.y, b2.max.y, b3.max.y, b4.max.y},
		{b1.min.z, b2.min.z, b3.min.z, b4.min.z},
		{b1.max.z, b2.max.z, b3.max.z, b4.max.z}
	}
	{}

	BBox4 operator+(const BBox4& b) const {
		BBox4 result;
		for (int i = 0; i < 6; ++i)
			result.bounds[i] = bounds[i] + b.bounds[i];
		return result;
	}

	BBox4 operator-(const BBox4& b) const {
		BBox4 result;
		for (int i = 0; i < 6; ++i)
			result.bounds[i] = bounds[i] - b.bounds[i];
		return result;
	}

	BBox4 operator*(const BBox4& b) const {
		BBox4 result;
		for (int i = 0; i < 6; ++i)
			result.bounds[i] = bounds[i] * b.bounds[i];
		return result;
	}

	BBox4 operator*(const float f) const {
		BBox4 result;
		for (int i = 0; i < 6; ++i)
			result.bounds[i] = bounds[i] * f;
		return result;
	}

	BBox4 operator/(const BBox4& b) const {
		BBox4 result;
		for (int i = 0; i < 6; ++i)
			result.bounds[i] = bounds[i] / b.bounds[i];
		return result;
	}

	BBox4 operator/(const float f) const {
		BBox4 result;
		for (int i = 0; i < 6; ++i)
			result.bounds[i] = bounds[i] / f;
		return result;
	}

	// Union
	BBox4 operator|(const BBox4& b) {
		BBox4 result;
		for (int i = 0; i < 3; ++i) {
			const int i1 = i * 2;
			const int i2 = i1 + 1;
			result.bounds[i1] = SIMD::min(bounds[i1], b.bounds[i1]);
			result.bounds[i2] = SIMD::max(bounds[i1], b.bounds[i1]);
		}
		return result;
	}

	// Intersection
	BBox4 operator&(const BBox4& b) {
		BBox4 result;
		for (int i = 0; i < 3; ++i) {
			const int i1 = i * 2;
			const int i2 = i1 + 1;
			result.bounds[i1] = SIMD::max(bounds[i1], b.bounds[i1]);
			result.bounds[i2] = SIMD::min(bounds[i1], b.bounds[i1]);
		}
		return result;
	}

	/**
	 * @brief Merge another BBox4 into this one.
	 *
	 * Merges another BBox4 into this one, resulting in a BBox4 that fully
	 * encompasses both.
	 */
	BBox4& merge_with(const BBox4& b) {
		for (int i = 0; i < 3; ++i) {
			const int i1 = i * 2;
			const int i2 = i1 + 1;
			bounds[i1] = SIMD::min(bounds[i1], b.bounds[i1]);
			bounds[i2] = SIMD::max(bounds[i1], b.bounds[i1]);
		}
		return *this;
	}

	/**
	 * @brief Tests a ray against the BBox4's bounding boxes.
	 *
	 * @param[in] o The origin of the ray, laid out as [[x,x,x,x],[y,y,y,y],[z,z,z,z]].
	 * @param[in] d_inv The direction of the ray over 1.0, laid out as [[x,x,x,x],[y,y,y,y],[z,z,z,z]]
	 * @param[in] t_max The maximum t value of the ray being tested against, laid out as [t,t,t,t].
	 * @param[in] d_sign Precomputed values indicating whether the x, y, and z components of the ray are negative or not.
	 * @param[out] hit_ts Pointer to a SIMD::float4 where the t parameter of each hit (if any) will be recorded..
	 *
	 * @returns A bitmask indicating which (if any) of the four boxes were hit.
	 */
	inline unsigned int intersect_ray(const SIMD::float4* o, const SIMD::float4* d_inv, const SIMD::float4& t_max, const std::array<uint32_t, 3>& d_sign, SIMD::float4 *hit_ts) const {
		using namespace SIMD;
		const float4 zeros(0.0f);

		// Calculate the plane intersections
		const float4 xlos = (bounds[0+d_sign[0]] - o[0]) * d_inv[0];
		const float4 xhis = (bounds[1-d_sign[0]] - o[0]) * d_inv[0];
		const float4 ylos = (bounds[2+d_sign[1]] - o[1]) * d_inv[1];
		const float4 yhis = (bounds[3-d_sign[1]] - o[1]) * d_inv[1];
		const float4 zlos = (bounds[4+d_sign[2]] - o[2]) * d_inv[2];
		const float4 zhis = (bounds[5-d_sign[2]] - o[2]) * d_inv[2];

		// Get the minimum and maximum hits
		const float4 mins = max(max(xlos, ylos), max(zlos, zeros));
		const float4 maxs = min(min(xhis, yhis), zhis);

		// Check for hits
		const float4 hits = lt(mins, t_max) && lte(mins, maxs);

		// Fill in near hits
		*hit_ts = mins;

		return to_bitmask(hits);
	}


	inline unsigned int intersect_ray(const Ray& ray, SIMD::float4 *hit_ts) const {
		using namespace SIMD;
		const Vec3 d_inv_f = ray.get_d_inverse();
		const std::array<uint32_t, 3> d_sign = ray.get_d_sign();

		// Load ray origin, inverse direction, and max_t into simd layouts for intersection testing
		const float4 ray_o[3] = {ray.o[0], ray.o[1], ray.o[2]};
		const float4 d_inv[3] = {d_inv_f[0], d_inv_f[1], d_inv_f[2]};
		const float4 max_t {
			ray.max_t
		};

		return intersect_ray(ray_o, d_inv, max_t, d_sign, hit_ts);
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

	void init(const uint8_t &state_count_) {
		bbox.init(state_count_);
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
		if (bbox.size() == b.bbox.size()) {
			for (int i=0; i < bbox.size(); i++) {
				bbox[i].merge_with(b.bbox[i]);
			}
		}
		// BBoxes have differing state count, so we
		// merge into a single-state bbox.
		// TODO: something more sophisticated.
		else {
			BBox bb = bbox[0];
			for (int i=1; i < bbox.size(); i++)
				bb.merge_with(bbox[i]);
			for (int i=0; i < b.bbox.size(); i++)
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

