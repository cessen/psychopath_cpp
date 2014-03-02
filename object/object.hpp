#ifndef OBJECT_HPP
#define OBJECT_HPP

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
 * @brief Base object class, from which all other objects inherit.
 */
class Object
{
public:
	virtual ~Object() {}

	/**
	 * @brief An enum type for describing the type of an object.
	 */
	enum Type {
	    SURFACE,
	    DICEABLE_SURFACE
	};

	// Unique ID, used by Scene and Tracer for various purposes
	// Sub-classes should ignore it.
	size_t uid;

	/**
	 * @brief Returns the type of the object.
	 */
	virtual Type get_type() const = 0;

	/**
	 * @brief Returns the bounds of the object.
	 */
	virtual BBoxT &bounds() = 0;
};


/**
 * @brief An interface for traditional surface objects.
 */
class Surface: public Object
{
public:
	virtual ~Surface() {}

	Object::Type get_type() const final {
		return Object::SURFACE;
	}

	/**
	 * @brief Tests a ray against the surface.
	 */
	virtual bool intersect_ray(const Ray &ray, Intersection *intersection=nullptr) = 0;
};


/**
 * @brief An interface for diceable surface objects.
 */
class DiceableSurface: public Object
{
public:
	virtual ~DiceableSurface() {}

	Object::Type get_type() const final {
		return DICEABLE_SURFACE;
	}

	/**
	 * @brief Returns the number of subdivisions necessary to achieve the
	 * given target width of microgeometry.
	 */
	virtual size_t subdiv_estimate(float width) const = 0;

	/**
	 * @brief Returns a pointer to a heap-allocated duplicate of the surface.
	 */
	virtual std::unique_ptr<DiceableSurface> copy() = 0;

	/**
	 * @brief Splits a diceable surface into two or more sub-surfaces.
	 * Splitting MUST be deterministic: given the same surface, splitting
	 * should result in the same output surfaces in the same order.
	 *
	 * Places pointers to the surfaces in the given surface pointer array.
	 *
	 * @warning To implementors: the implementation of this method must allow
	 * the surface itself to be replaced by one of the new surfaces.  So make
	 * sure not to assign to the array until you don't need the surface's data
	 * anymore.
	 *
	 * @return The number of new surfaces generated from the split
	 */
	virtual int split(std::unique_ptr<DiceableSurface> objects[]) = 0;

	/**
	 * @brief Dices the surface into a MicroSurface.
	 *
	 * @param subdivisions The number of subdivisions to dice it to.  For most
	 *        subdivision schemes, the amount of geometry quadruples every
	 *        subdivision iteration.
	 */
	virtual std::shared_ptr<MicroSurface> dice(size_t subdivisions) = 0;
};

#endif // OBJECT_HPP
