#ifndef SPHERE_HPP
#define SPHERE_HPP

#include "numtype.h"

#include <vector>
#include "vector.hpp"
#include "primitive.hpp"
#include "timebox.hpp"

/**
 * @brief A sphere primitive.
 *
 * This serves as a simple example of how to implement a surface primitive.
 */
class Sphere: public Surface
{
public:
	TimeBox<Vec3> center;
	TimeBox<float32> radius;

	BBoxT bbox;
	bool has_bounds;


	Sphere(Vec3 center_, float32 radius_);
	Sphere(uint8 res_time_);
	virtual ~Sphere();

	void add_time_sample(int samp, Vec3 center_, float32 radius_);

	virtual bool intersect_ray(Ray &ray, Intersection *intersection=NULL);
	virtual BBoxT &bounds();
	bool is_traceable(float32 ray_width) {
		return true;
	}
	virtual void refine(std::vector<Primitive *> &primitives) {
		return;
	}
};

#endif
