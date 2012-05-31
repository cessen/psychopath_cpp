#ifndef SPHERE_HPP
#define SPHERE_HPP

#include "numtype.h"

#include <vector>
#include "vector.hpp"
#include "primitive.hpp"
#include "timebox.hpp"

/*
 * A sphere.
 */
class Sphere: public Surface
{
public:
	TimeBox<Vec3> center;
	TimeBox<float32> radius;

	BBox bbox;
	bool has_bounds;

	Sphere(uint8 res_time_);
	Sphere(Vec3 center_, float32 radius_);
	virtual ~Sphere();

	void add_time_sample(int samp, Vec3 center_, float32 radius_);

	virtual bool intersect_ray(Ray &ray, Intersection *intersection=NULL);
	virtual BBox &bounds();
	bool is_traceable(float32 ray_width) {
		return true;
	}
	virtual void refine(std::vector<Primitive *> &primitives) {
		return;
	}
};

#endif
