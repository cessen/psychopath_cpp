#ifndef PRIMITIVE_HPP
#define PRIMITIVE_HPP

#include "numtype.h"

#include <memory>
#include <vector>
#include <iostream>
#include <stdlib.h>
#include "ray.hpp"
#include "intersection.hpp"
#include "bbox.hpp"
#include "micro_surface.hpp"


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
 * @brief An interface for primitives.
 *
 * Primitives are anything that can be bounded in space and rendered
 * in some fashion.  E.g. surfaces, particles, volumes, etc.
 *
 * Every distinct primitive has a unique ID that identifies it in
 * the rendering system for various purposes.
 *
 * Some primitives may need to be separated into natural sub-components
 * before being rendered.
 */
class Primitive: public Boundable
{
public:
	size_t uid; // Unique ID, used by Scene and Tracer for various purposes
	// Sub-classes don't need to worry about it.

	virtual ~Primitive() {}

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
};


/**
 * @brief An interface for diceable surface primitives.
 */
class DiceableSurfacePrimitive: public Primitive
{
public:
	virtual ~DiceableSurfacePrimitive() {}

	/**
	 * @brief Returns the number of subdivisions necessary to achieve the
	 * given target width of microgeometry.
	 */
	virtual size_t subdiv_estimate(float width) const = 0;

	/**
	 * @brief Returns a pointer to a heap-allocated duplicate of the primitive.
	 */
	virtual std::unique_ptr<DiceableSurfacePrimitive> copy() = 0;

	/**
	 * @brief Splits a primitive into two or more sub-primitives.  Splitting MUST be
	 * deterministic: given the same primitive, splitting should result in the
	 * same output primitives in the same order.
	 *
	 * Places pointers to the primitives in the given primitives pointer array.
	 *
	 * @warning To implementors: the implementation of this method must allow
	 * the primitive itself to be replaced by one of the new primitives.  So make
	 * sure not to assign to the array until you don't need the primitive's data
	 * anymore.
	 *
	 * @return The number of new primitives generated from the split
	 */
	virtual int split(std::unique_ptr<DiceableSurfacePrimitive> primitives[]) = 0;

	/**
	 * @brief Dices the surface into a MicroSurface.
	 *
	 * @param subdivisions The number of subdivisions to dice it to.  For most
	 *        subdivision schemes, the amount of geometry quadruples every
	 *        subdivision iteration.
	 */
	virtual std::shared_ptr<MicroSurface> dice(size_t subdivisions) = 0;
};

#endif
