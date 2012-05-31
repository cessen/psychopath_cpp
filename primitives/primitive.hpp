#ifndef PRIMITIVE_HPP
#define PRIMITIVE_HPP

#include "numtype.h"

#include <vector>
#include <iostream>
#include <stdlib.h>
#include "ray.hpp"
#include "intersection.hpp"
#include "bbox.hpp"


/*
 * An interface for things that can be bound in space.
 */
class Boundable
{
public:
	virtual ~Boundable() {}

	/* Calculates a bounding box for the object.
	 * Stored in bbox. */
	virtual BBox &bounds() = 0;
};


/*
 * An interface for things that can be ray traced against.
 */
class Traceable
{
public:
	virtual ~Traceable() {}

	/* Intersects a ray with the primitive.
	 * Intersection information gets stored in the optional
	 * "intersection" structure.
	 * Returns true on a hit, false on a miss.
	 */
	virtual bool intersect_ray(Ray &ray, Intersection *intersection=NULL) = 0;
};


/*
 * An interface for primitives.
 * Primitives are anything that can be (eventually) raytraced
 * for rendering and which can be bounded in space.  E.g. surfaces,
 * particles, volumes, etc.
 *
 * Some primitives may need to be split into sub-primitives first,
 * but should eventually split down to something directly traceable.
 */
class Primitive: public Boundable, public Traceable
{
public:
	virtual ~Primitive() {}

	/* Splits a primitive into several sub-primitives.
	   Places pointers to the primitives in the given primitives vector.
	 */
	virtual void refine(std::vector<Primitive *> &primitives) {
		std::cout << "Error: Primitive::refine() not implemented for this primitive." << std::endl;
		exit(1);
	}

	/* Returns whether the surface needs to be split before tracing.
	 */
	virtual bool is_traceable(float32 ray_width) {
		return true;
	}
};


/*
 * An interface for surface primitives.
 */
class Surface: public Primitive
{
public:
	virtual ~Surface() {}
};

#endif
