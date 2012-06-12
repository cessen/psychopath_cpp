#ifndef RAYINTER_HPP
#define RAYINTER_HPP

#include "numtype.h"
#include "ray.hpp"
#include "intersection.hpp"

/**
 * @brief A struct containing both a ray and an intersection.
 *
 * Primarily used to pass data back-and-forth between the Integrator
 * and Tracer.
 */
struct RayInter {
	Ray ray;
	Intersection inter;
	bool hit;
	uint32 id;
};

#endif // RAYINTER_HPP
