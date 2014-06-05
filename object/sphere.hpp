#ifndef SPHERE_HPP
#define SPHERE_HPP

#include "numtype.h"

#include <vector>
#include "vector.hpp"
#include "object.hpp"

/**
 * @brief A sphere primitive.
 *
 * This serves as a simple example of how to implement a surface primitive.
 */
class Sphere: public Surface
{
public:
	std::vector<Vec3> center;
	std::vector<float> radius;

	std::vector<BBox> bbox;
	bool has_bounds;


	Sphere(Vec3 center_, float radius_);
	Sphere(uint8_t res_time_);
	virtual ~Sphere() {};

	void add_time_sample(int samp, Vec3 center_, float radius_);

	virtual bool intersect_ray(const Ray &ray, Intersection *intersection=nullptr);
	virtual std::vector<BBox> &bounds();
};

#endif
