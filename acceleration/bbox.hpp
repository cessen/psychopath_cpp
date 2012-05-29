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

struct BBounds {
	Vec3 min;
	Vec3 max;
};

struct BBox {
public:
	TimeBox<Vec3> bmin;
	TimeBox<Vec3> bmax;

	BBox(const int32 &res_time=1);
	BBox(const Vec3 &bmin_, const Vec3 &bmax_);

	void add_time_sample(const int32 &samp, const Vec3 &bmin_, const Vec3 &bmax_);

	bool intersect_ray_(Ray &ray, float32 *hitt0=NULL, float32 *hitt1=NULL);

	/*
	 * Copies another BBox into this one, overwriting any bounds
	 * that were already there.
	 */
	void copy(const BBox &b);

	/*
	 * Merges another BBox into this one, resulting in a new minimal BBox
	 * that contains both the originals.
	 * Only works on BBoxes with the same time resolution at the moment.
	 */
	void merge(const BBox &b);

	/*
	 * Returns the surface area of the BBox.
	 * For now just takes the first time sample.
	 */
	float32 surface_area() const {
		const float32 x = bmax[0].x - bmin[0].x;
		const float32 y = bmax[0].y - bmin[0].y;
		const float32 z = bmax[0].z - bmin[0].z;

		return 2 * (x*y + x*z + y*z);
	}
};









//#define BBOXSSE
#ifdef BBOXSSE
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
#else
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
