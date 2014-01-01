#ifndef PRIMITIVE_HPP
#define PRIMITIVE_HPP

#include "numtype.h"

#include <vector>
#include <iostream>
#include <stdlib.h>
#include "ray.hpp"
#include "intersection.hpp"
#include "bbox.hpp"


/**
 * @brief An interface for things that can be bound in space.
 */
class Boundable
{
public:
	virtual ~Boundable() {}

	/**
	 @brief Returns the bounding box of the object.
	 */
	virtual BBoxT &bounds() = 0;
};


/**
 * @brief An interface for things that can be ray traced against.
 */
class Traceable
{
public:
	virtual ~Traceable() {}

	/**
	 @brief Intersects a ray with the primitive.

	 Intersection information gets stored in the optional
	 "intersection" structure.

	 @param[in] ray Ray to test against.
	 @param[out] intersection Resulting intersection, if there is one.

	 @return True on a hit, false on a miss.
	 */
	virtual bool intersect_ray(const Ray &ray, Intersection *intersection=nullptr) = 0;
};


/**
 * @brief An interface for primitives.
 *
 * Primitives are anything that can be (eventually) raytraced
 * for rendering and which can be bounded in space.  E.g. surfaces,
 * particles, volumes, etc.
 *
 * Some primitives may need to be separated into sub-primitives first,
 * but should eventually separate into something traceable.
 */
class Primitive: public Boundable, public Traceable
{
public:
	size_t id {++Global::next_primitive_id};

	virtual ~Primitive() {}

	/**
	 @brief Returns whether the primitive needs to be separated before tracing.
	 */
	virtual bool is_traceable() {
		return true;
	}

	/**
	 @brief Separates a primitive into natural sub-primitives.

	 Places pointers to the primitives in the given primitives vector.
	 */
	virtual void separate(std::vector<Primitive *> &primitives) {
		std::cout << "Error: Primitive::separate() not implemented for this primitive." << std::endl;
		exit(1);
	}
};


/**
 * @brief An interface for surface primitives.
 */
class SurfacePrimitive: public Primitive
{
public:
	virtual ~SurfacePrimitive() {}

	/**
	 @brief Splits a primitive into two or more sub-primitives.

	 Places pointers to the primitives in the given primitives vector.
	 */
	virtual void split(std::vector<Primitive *> &primitives) {
		std::cout << "Error: Primitive::split() not implemented for this primitive." << std::endl;
		exit(1);
	}
};

#endif
