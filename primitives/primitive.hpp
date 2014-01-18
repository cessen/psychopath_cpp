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
	size_t uid {++Global::next_primitive_uid};

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
	virtual size_t subdiv_estimate(float width) const {
		std::cout << "Error: DiceableSurfacePrimitive::query_subdivision_rate() not implemented for this primitive." << std::endl;
		exit(1);
	}

	/**
	 * @brief Splits a primitive into two or more sub-primitives.  Splitting MUST be
	 * deterministic: given the same primitive, splitting should result in the
	 * same output primitives in the same order.
	 *
	 * Places pointers to the primitives in the given primitives vector.
	 */
	virtual void split(std::vector<DiceableSurfacePrimitive *> &primitives) {
		std::cout << "Error: DiceableSurfacePrimitive::split() not implemented for this primitive." << std::endl;
		exit(1);
	}

	/**
	 * @brief Dices the surface into a MicroSurface.
	 *
	 * @param subdivisions The number of subdivisions to dice it to.  For most
	 *        subdivision schemes, the amount of geometry quadruples every
	 *        subdivision iteration.
	 */
	virtual std::shared_ptr<MicroSurface> dice(size_t subdivisions) {
		std::cout << "Error: DiceableSurfacePrimitive::dice() not implemented for this primitive." << std::endl;
		exit(1);
	}
};

#endif
