#ifndef BILINEAR_HPP
#define BILINEAR_HPP

#include "numtype.h"

#include <vector>
#include "vector.hpp"
#include "grid.hpp"
#include "primitive.hpp"
#include "timebox.hpp"
#include "grid_cache.hpp"

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
public:
	TimeBox<Vec3 *> verts;
	GridCacheKey grid_key;

	BBox bbox;
	bool has_bounds;

	float32 last_rayw;

	Bilinear(uint8 res_time_);
	Bilinear(Vec3 v1, Vec3 v2, Vec3 v3, Vec3 v4);
	virtual ~Bilinear();

	virtual bool intersect_ray(Ray &ray, Intersection *intersection=NULL);
	virtual BBox &bounds();
	virtual int dice_rate(float32 upoly_width);
	bool is_traceable(float32 ray_width);
	virtual void refine(std::vector<Primitive *> &primitives);

	void add_time_sample(int samp, Vec3 v1, Vec3 v2, Vec3 v3, Vec3 v4);
	void dice(const int ru, const int rv);
};

#endif
