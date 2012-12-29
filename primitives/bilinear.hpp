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
public:
	TimeBox<Vec3 *> verts;
	MicroSurfaceCacheKey microsurface_key;

	BBoxT bbox;
	bool has_bounds;

	Bilinear(uint16 res_time_);
	Bilinear(Vec3 v1, Vec3 v2, Vec3 v3, Vec3 v4);
	virtual ~Bilinear();

	virtual bool intersect_ray(Ray &ray, Intersection *intersection=NULL);
	virtual BBoxT &bounds();

	bool is_traceable();
	virtual void split(std::vector<Primitive *> &primitives);

	virtual uint_i micro_estimate(float32 width);
	virtual MicroSurface *micro_generate(float32 width);

	void add_time_sample(int samp, Vec3 v1, Vec3 v2, Vec3 v3, Vec3 v4);
	Grid *dice(const int ru, const int rv);
};

#endif
