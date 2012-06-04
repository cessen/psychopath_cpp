#ifndef BBOX_HPP
#define BBOX_HPP

#include "numtype.h"

#include <stdlib.h>
#include <xmmintrin.h>
#include "timebox.hpp"
#include "vector.hpp"
#include "ray.hpp"
#include "utils.hpp"

union V4 {
	float32 a[4];
	__m128 b;
};

/**
 * @brief A single axis-aligned bounding box.
 */
struct BBox {
	Vec3 min;
	Vec3 max;
	
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
	 * @brief Sets this bbox to be equal to the given bbox.
	 */
	void operator=(const BBox &b) {
		min = b.min;
		max = b.max;
	}
	
	/**
	 * @brief Merge another BBox into this one.
	 *
	 * Merges another BBox into this one, resulting in a BBox that fully
	 * encompasses both.
	 */
	void merge_with(const BBox &b) {
		if (b.min.x < min.x)
			min.x = b.min.x;
		if (b.min.y < min.y)
			min.y = b.min.y;
		if (b.min.z < min.z)
			min.z = b.min.z;
		
		if (b.max.x > max.x)
			max.x = b.max.x;
		if (b.max.y > max.y)
			max.y = b.max.y;
		if (b.max.z > max.z)
			max.z = b.max.z;
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
			*hitt0 = tmin;
			*hitt1 = tmax;
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

	void add_time_sample(const int32 &samp, const Vec3 &bmin_, const Vec3 &bmax_)
	{
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
	
	BBox &operator[](const int32 &i) {
		return bbox.states[i];
	}

	const BBox &operator[](const int32 &i) const {
		return bbox.states[i];
	}

	/*
	 * Copies another BBox into this one, overwriting any bounds
	 * that were already there.
	 */
	void copy(const BBoxT &b);

	/**
	 * Merges another BBox into this one, resulting in a new minimal BBox
	 * that contains both the originals.
	 */
	void merge_with(const BBoxT &b) {
		// BBoxes have the same state count, so we
		// can just merge each corresponding state.
		if (bbox.state_count == b.bbox.state_count) {
			for(int i=0; i < bbox.state_count; i++)
			{
				bbox[i].merge_with(b.bbox[i]);
			}
		}
		// BBoxes have differing state count, so we
		// merge into a single-state bbox.
		// TODO: something more sophisticated.
		else
		{
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










#if 0
static inline bool fast_intersect_test_bbox(const BBox &b, const Ray &ray, float32 &tmin, float32 &tmax)
{
	//std::cout << "Hi" << std::endl;
	int32 ia=0, ib=0;
	float32 alpha=0.0;
	__m128 bmin, bmax;

	// Calculate bounds in time
	if (b.bmin.query_time(ray.time, &ia, &ib, &alpha)) {
		bmin = _mm_setr_ps(lerp(alpha, b.bmin[ia].x, b.bmin[ib].x),
		                   lerp(alpha, b.bmin[ia].y, b.bmin[ib].y),
		                   lerp(alpha, b.bmin[ia].z, b.bmin[ib].z),
		                   0.0);
		bmax = _mm_setr_ps(lerp(alpha, b.bmax[ia].x, b.bmax[ib].x),
		                   lerp(alpha, b.bmax[ia].y, b.bmax[ib].y),
		                   lerp(alpha, b.bmax[ia].z, b.bmax[ib].z),
		                   0.0);
	} else {
		bmin = _mm_setr_ps(b.bmin[0].x, b.bmin[0].y, b.bmin[0].z, 0.0);
		bmax = _mm_setr_ps(b.bmax[0].x, b.bmax[0].y, b.bmax[0].z, 0.0);
	}

	// Fetch the ray origin and inverse direction
	const __m128 o = _mm_setr_ps(ray.o.x, ray.o.y, ray.o.z, 0.0);
	const __m128 invd = _mm_setr_ps(ray.inv_d.x, ray.inv_d.y, ray.inv_d.z, 0.0);




	// Calculate hit distances
	const __m128 temp1 = _mm_mul_ps(_mm_sub_ps(bmin, o), invd);
	const __m128 temp2 = _mm_mul_ps(_mm_sub_ps(bmax, o), invd);

	// Calculate near and far hit distances
	V4 t_nears, t_fars;
	t_nears.b = _mm_min_ps(temp1, temp2);
	t_fars.b = _mm_max_ps(temp1, temp2);

	const float32 temp3 = t_nears.a[0] > t_nears.a[1] ? t_nears.a[0] : t_nears.a[1];
	tmin = temp3 > t_nears.a[2] ? temp3 : t_nears.a[2];

	const float32 temp4 = t_fars.a[0] < t_fars.a[1] ? t_fars.a[0] : t_fars.a[1];
	tmax = temp4 < t_fars.a[2] ? temp4 : t_fars.a[2];

	//std::cout << (tmin < tmax) << " " << tmin << " " << tmax << std::endl;
	//float32 tmin2 = tmin;
	//float32 tmax2 = tmax;
	//bool hit = fast_intersect_test_bbox2(b, ray, tmin2, tmax2);
	//std::cout << hit << " " << tmin2 << " " << tmax2 << std::endl;

	return (tmin < tmax) && (tmin < ray.max_t) && (tmax > ray.min_t);
}

static inline bool fast_intersect_test_bbox(const BBox &b, const Ray &ray, float32 &tmin, float32 &tmax)
{
	int32 ia=0, ib=0;
	float32 alpha=0.0;
	Vec3 bounds[2];

	// Calculate bounds in time
	if (b.bmin.query_time(ray.time, &ia, &ib, &alpha)) {
		bounds[0].x = lerp(alpha, b.bmin[ia].x, b.bmin[ib].x);
		bounds[1].x = lerp(alpha, b.bmax[ia].x, b.bmax[ib].x);
		bounds[0].y = lerp(alpha, b.bmin[ia].y, b.bmin[ib].y);
		bounds[1].y = lerp(alpha, b.bmax[ia].y, b.bmax[ib].y);
		bounds[0].z = lerp(alpha, b.bmin[ia].z, b.bmin[ib].z);
		bounds[1].z = lerp(alpha, b.bmax[ia].z, b.bmax[ib].z);
	} else {
		bounds[0] = b.bmin[0];
		bounds[1] = b.bmax[0];
	}

	tmin = (bounds[ray.d_is_neg[0]].x - ray.o.x) * ray.inv_d.x;
	tmax = (bounds[1-ray.d_is_neg[0]].x - ray.o.x) * ray.inv_d.x;
	const float32 tymin = (bounds[ray.d_is_neg[1]].y - ray.o.y) * ray.inv_d.y;
	const float32 tymax = (bounds[1-ray.d_is_neg[1]].y - ray.o.y) * ray.inv_d.y;
	const float32 tzmin = (bounds[ray.d_is_neg[2]].z - ray.o.z) * ray.inv_d.z;
	const float32 tzmax = (bounds[1-ray.d_is_neg[2]].z - ray.o.z) * ray.inv_d.z;

	// The if statement version seems to be slightly faster for some reason.
	//tmin = tmin > tymin ? tmin : tymin;
	//tmin = tmin > tzmin ? tmin : tzmin;
	//tmax = tmax < tymax ? tmax : tymax;
	//tmax = tmax < tzmax ? tmax : tzmax;
	if (tymin > tmin)
		tmin = tymin;
	if (tzmin > tmin)
		tmin = tzmin;
	if (tymax < tmax)
		tmax = tymax;
	if (tzmax < tmax)
		tmax = tzmax;

	return (tmin < tmax) && (tmin < ray.max_t) && (tmax > ray.min_t);
}
#endif

#endif
