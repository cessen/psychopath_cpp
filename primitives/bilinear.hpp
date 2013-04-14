#ifndef BILINEAR_HPP
#define BILINEAR_HPP

#include "numtype.h"

#include <vector>
#include "vector.hpp"
#include "grid.hpp"
#include "primitive.hpp"
#include "timebox.hpp"
#include "micro_surface_cache.hpp"

/*
 * A bilinear patch.
 * Vertices arranged like this:
 *     u-->
 *   v1----v2
 * v  |    |
 * | v4----v3
 * \/
 */
class Bilinear: public Surface
{
	void uv_dice_rate(size_t *u_rate, size_t *v_rate, float width) {
		// longest u-side  and v-side of the patch
		const float ul = (verts[0][0] - verts[0][1]).length() > (verts[0][2] - verts[0][3]).length() ? (verts[0][0] - verts[0][1]).length() : (verts[0][2] - verts[0][3]).length();
		const float vl = (verts[0][0] - verts[0][3]).length() > (verts[0][1] - verts[0][2]).length() ? (verts[0][0] - verts[0][3]).length() : (verts[0][1] - verts[0][2]).length();

		// Dicing rates in u and v based on target microelement width
		*u_rate = ul / (width * Config::dice_rate);
		if (*u_rate < 1)
			*u_rate = 1;
		*v_rate = vl / (width * Config::dice_rate);
		if (*v_rate < 1)
			*v_rate = 1;

		//if(*u_rate == 1 && *v_rate == 1)
		//	std::cout << width << " " << *u_rate << " " << *v_rate << std::endl;
	}

public:
	TimeBox<Vec3 *> verts;
	float u_min, v_min, u_max, v_max;
	MicroSurfaceCacheKey microsurface_key;
	float last_ray_width;


	BBoxT bbox;
	bool has_bounds;

	Bilinear(uint16_t res_time_);
	Bilinear(Vec3 v1, Vec3 v2, Vec3 v3, Vec3 v4);
	virtual ~Bilinear();

	virtual bool intersect_ray(const Ray &ray, Intersection *intersection=nullptr);
	virtual BBoxT &bounds();

	bool is_traceable();
	virtual void split(std::vector<Primitive *> &primitives);

	virtual size_t micro_estimate(float width);
	virtual MicroSurface *micro_generate(float width);

	void add_time_sample(int samp, Vec3 v1, Vec3 v2, Vec3 v3, Vec3 v4);
	Grid *dice(const int ru, const int rv);
};

#endif
