#ifndef SURFACE_HPP
#define SURFACE_HPP

#include <vector>

#include "primitive.hpp"

#include "spinlock.hpp"

/**
 * @brief A surface description.
 *
 * This is higher-level than a SurfacePrimitive.  A SurfacePrimitive
 * only describes geometry, whereas a Surface describes shading
 * as well.
 *
 * The Surface class also transparently handles splitting internally,
 * when necessary to achieve the desired dicing rates.
 */
class Surface: Primitive
{
	// Shader *shader;
	std::vector<SurfacePrimitive*> surfaces;
	Spinlock lock;

	Surface(): {}

	virtual bool intersect_ray(const Ray &ray, Intersection *intersection=nullptr) {
		lock.lock();

		bool result = false;
		for (auto surface: surfaces) {
			result = result && surface->intersect_ray(ray, intersection);
		}

		lock.unlock();
		return result;
	}
};

#endif // SURFACE_HPP