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
class Sphere final: public Surface
{
public:
	std::vector<Vec3> center;
	std::vector<float> radius;

	std::vector<BBox> bbox;

	Sphere(Vec3 center_, float radius_);
	Sphere(uint8_t res_time_);
	virtual ~Sphere() {};

	void add_time_sample(int samp, Vec3 center_, float radius_);

	void finalize();

	virtual bool intersect_ray(const Ray &ray, Intersection *intersection=nullptr);
	virtual const std::vector<BBox> &bounds() const;
	virtual Color total_emitted_color() const override final {
		return Color(0.0f);
	}
};

#endif
