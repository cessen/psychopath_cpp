#ifndef BBOX_HPP
#define BBOX_HPP

#include "numtype.h"

#include <limits>
#include <iostream>
#include <cmath>
#include <stdlib.h>
#include <tuple>
#include <array>

#include "simd.hpp"
#include "global.hpp"
#include "vector.hpp"
#include "ray.hpp"
#include "transform.hpp"
#include "utils.hpp"

#define BBOX_MAXT_ADJUST 1.00000024f

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

	BBox(const BBox& b): min {b.min}, max {b.max} {}

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
	BBox operator|(const BBox& b) const {
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
	BBox operator&(const BBox& b) const {
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
	 * @param[in] t an optional alternative ray max_t to use.
	 *
	 * @returns True if the ray hits, false if the ray misses.
	 */
	inline bool intersect_ray(const Ray& ray, float *hitt0, float *hitt1, const float t) const {
#ifdef DEBUG
		// Test for nan and inf
		if (std::isnan(ray.o.x) || std::isnan(ray.o.y) || std::isnan(ray.o.z) ||
		        std::isnan(ray.d.x) || std::isnan(ray.d.y) || std::isnan(ray.d.z) ||
		        std::isnan(ray.d_inv.x) || std::isnan(ray.d_inv.y) || std::isnan(ray.d_inv.z) ||
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

		const auto d_sign = ray.d_sign();

		// Find slab intersections
		const float txmin = (bounds[d_sign[0]].x - ray.o.x) * ray.d_inv.x;
		const float txmax = (bounds[1-d_sign[0]].x - ray.o.x) * ray.d_inv.x;
		const float tymin = (bounds[d_sign[1]].y - ray.o.y) * ray.d_inv.y;
		const float tymax = (bounds[1-d_sign[1]].y - ray.o.y) * ray.d_inv.y;
		const float tzmin = (bounds[d_sign[2]].z - ray.o.z) * ray.d_inv.z;
		const float tzmax = (bounds[1-d_sign[2]].z - ray.o.z) * ray.d_inv.z;

		// Calculate tmin
		const float tmin1 = txmin > tymin ? txmin : tymin;
		const float tmin2 = tzmin > 0.0f ? tzmin : 0.0f;
		*hitt0 = tmin1 > tmin2 ? tmin1 : tmin2;

		// Calculate tmax
		const float tmax1 = txmax < tymax ? txmax : tymax;
		const float tmax2 = tzmax < t ? tzmax :t;
		*hitt1 = (tmax1 < tmax2 ? tmax1 : tmax2) * BBOX_MAXT_ADJUST;

		// Did we hit?
		return *hitt0 <= *hitt1;
	}

	inline bool intersect_ray(const Ray& ray, float *hitt0, float *hitt1) const {
		return intersect_ray(ray, hitt0, hitt1, ray.max_t);
	}

	inline bool intersect_ray(const Ray& ray) const {
		float hitt0, hitt1;
		return intersect_ray(ray, &hitt0, &hitt1, ray.max_t);
	}

	/**
	 * @brief Creates a new BBox transformed into a different space.
	 *
	 * TODO: there is a more efficient, though less intuitive, algorithm for
	 * this.  Implement it.
	 */
	BBox transformed(const Transform& trans) const {
		// BBox corners
		std::array<Vec3,8> vs {{
				Vec3(min[0], min[1], min[2]),
				Vec3(min[0], min[1], max[2]),
				Vec3(min[0], max[1], min[2]),
				Vec3(min[0], max[1], max[2]),
				Vec3(max[0], min[1], min[2]),
				Vec3(max[0], min[1], max[2]),
				Vec3(max[0], max[1], min[2]),
				Vec3(max[0], max[1], max[2])
			}
		};

		// Transform BBox corners
		for (auto& v: vs)
			v = trans.pos_to(v);

		// Find the min and max
		BBox b;
		for (auto& v: vs) {
			for (int i = 0; i < 3; i++) {
				b.min[i] = v[i] < b.min[i] ? v[i] : b.min[i];
				b.max[i] = v[i] > b.max[i] ? v[i] : b.max[i];
			}
		}

		return b;
	}

	BBox inverse_transformed(const Transform& trans) const {
		// BBox corners
		std::array<Vec3,8> vs {{
				Vec3(min[0], min[1], min[2]),
				Vec3(min[0], min[1], max[2]),
				Vec3(min[0], max[1], min[2]),
				Vec3(min[0], max[1], max[2]),
				Vec3(max[0], min[1], min[2]),
				Vec3(max[0], min[1], max[2]),
				Vec3(max[0], max[1], min[2]),
				Vec3(max[0], max[1], max[2])
			}
		};

		// Transform BBox corners
		for (auto& v: vs)
			v = trans.pos_from(v);

		// Find the min and max
		BBox b;
		for (auto& v: vs) {
			for (int i = 0; i < 3; i++) {
				b.min[i] = v[i] < b.min[i] ? v[i] : b.min[i];
				b.max[i] = v[i] > b.max[i] ? v[i] : b.max[i];
			}
		}

		return b;
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
	inline unsigned int intersect_ray(const SIMD::float4* o, const SIMD::float4* d_inv, const SIMD::float4& t_max, const Ray::Signs& d_sign, SIMD::float4 *hit_ts) const {
		using namespace SIMD;
		const float4 zeros(0.0f);
		const float4 ninf(-std::numeric_limits<float>::infinity());

		// Calculate the plane intersections
		const float4 xs = (shuffle_swap(bounds[0], d_sign[0]) - o[0]) * d_inv[0];
		const float4 ys = (shuffle_swap(bounds[1], d_sign[1]) - o[1]) * d_inv[1];
		const float4 zs = (shuffle_swap(bounds[2], d_sign[2]) - o[2]) * d_inv[2];

		// Get the minimum and maximum hits, and shuffle the max hits
		// to be in the same location as the minimum hits
		const float4 mins = max(max(xs, ys), max(zs, zeros));
		const float4 maxs = shuffle_swap(max(min(min(xs, ys), zs), ninf)) * float4(BBOX_MAXT_ADJUST);

		// Check for hits
		const float4 hits = lt(mins, t_max) && lte(mins, maxs);

		// Fill in near hits
		*hit_ts = mins;

		return to_bitmask(hits) & 3;
	}


	inline unsigned int intersect_ray(const Ray& ray, SIMD::float4 *hit_ts) const {
		using namespace SIMD;
		const Vec3 d_inv_f = ray.get_d_inverse();
		const Ray::Signs d_sign = ray.d_sign();

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
	inline unsigned int intersect_ray(const SIMD::float4* o, const SIMD::float4* d_inv, const SIMD::float4& t_max, const Ray::Signs& d_sign, SIMD::float4 *hit_ts) const {
		using namespace SIMD;
		const float4 zeros(0.0f);
		const float4 ninf(-std::numeric_limits<float>::infinity());

		// Calculate the plane intersections
		const float4 xlos = (bounds[0+d_sign[0]] - o[0]) * d_inv[0];
		const float4 xhis = (bounds[1-d_sign[0]] - o[0]) * d_inv[0];
		const float4 ylos = (bounds[2+d_sign[1]] - o[1]) * d_inv[1];
		const float4 yhis = (bounds[3-d_sign[1]] - o[1]) * d_inv[1];
		const float4 zlos = (bounds[4+d_sign[2]] - o[2]) * d_inv[2];
		const float4 zhis = (bounds[5-d_sign[2]] - o[2]) * d_inv[2];

		// Get the minimum and maximum hits
		const float4 mins = max(max(xlos, ylos), max(zlos, zeros));
		const float4 maxs = max(min(min(xhis, yhis), zhis), ninf) * float4(BBOX_MAXT_ADJUST);

		// Check for hits
		const float4 hits = lt(mins, t_max) && lte(mins, maxs);

		// Fill in near hits
		*hit_ts = mins;

		return to_bitmask(hits);
	}


	inline unsigned int intersect_ray(const Ray& ray, SIMD::float4 *hit_ts) const {
		using namespace SIMD;
		const Vec3 d_inv_f = ray.get_d_inverse();
		const Ray::Signs d_sign = ray.d_sign();

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
 * Merges two vectors of BBoxes, interpreting the vectors as
 * being the BBoxes over time.  The result is a vector that
 * is the union of the two vectors of BBoxes.
 */
static inline std::vector<BBox> merge(const std::vector<BBox>& a, const std::vector<BBox>& b)
{
	std::vector<BBox> c;

	if (a.size() == 0) {
		c = b;
	} else if (b.size() == 0) {
		c = a;
	} else if (a.size() == b.size()) {
		for (size_t i = 0; i < a.size(); ++i)
			c.emplace_back(a[i] | b[i]);
	} else if (a.size() > b.size()) {
		const float s = a.size() - 1;
		for (size_t i = 0; i < a.size(); ++i)
			c.emplace_back(a[i] | lerp_seq(i/s, b.cbegin(), b.cend()));
	} else if (a.size() < b.size()) {
		const float s = b.size() - 1;
		for (size_t i = 0; i < b.size(); ++i)
			c.emplace_back(b[i] | lerp_seq(i/s, a.cbegin(), a.cend()));
	}

	return c;
}


static inline std::vector<BBox> transform_from(const std::vector<BBox>& bbs_in, std::vector<Transform>::const_iterator xstart, std::vector<Transform>::const_iterator xend)
{
	std::vector<BBox> bbs = bbs_in;

	// Transform the bounding boxes
	size_t tcount = std::distance(xstart, xend);

	if (tcount == 0) {
		// Do nothing
	} else if (bbs.size() == tcount) {
		for (size_t i = 0; i < bbs.size(); ++i)
			bbs[i] = bbs[i].inverse_transformed(xstart[i]);
	} else if (bbs.size() > tcount) {
		const float s = bbs.size() - 1;
		for (size_t i = 0; i < bbs.size(); ++i)
			bbs[i] = bbs[i].inverse_transformed(lerp_seq(i/s, xstart, xend));
	} else if (bbs.size() < tcount) {
		const float s = tcount - 1;
		std::vector<BBox> tbbs;
		for (size_t i = 0; i < tcount; ++i)
			tbbs.push_back(lerp_seq(i/s, bbs.cbegin(), bbs.cend()).inverse_transformed(xstart[i]));
		bbs = std::move(tbbs);
	}

	return bbs;
}


#endif // BBOX_HPP

