#ifndef OBJECT_HPP
#define OBJECT_HPP

#include "numtype.h"

#include <memory>
#include <vector>
#include <iostream>
#include <stdlib.h>
#include "stack.hpp"
#include "ray.hpp"
#include "intersection.hpp"
#include "bbox.hpp"
#include "transform.hpp"
#include "micro_surface.hpp"


/**
 * @brief Base object class, from which all other objects inherit.
 */
class Object
{
public:
	// Virtual destructor, and don't delete default copy/move constructors
	Object() = default;
	virtual ~Object() = default;
	Object(const Object&) = default;
	Object(Object&&) = default;
	Object& operator=(const Object&) = default;
	Object& operator=(Object&&) = default;

	/**
	 * @brief An enum type for describing the type of an object.
	 */
	enum Type {
	    SURFACE,
	    BREADTH_SURFACE,
	    DICEABLE_SURFACE,
	    LIGHT,
	    ASSEMBLY_INSTANCE
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
	virtual const std::vector<BBox> &bounds() const = 0;

	/**
	 * Returns the total amount of energy emitted by the object.
	 *
	 * This does not need to be 100% accurate, as it is only used
	 * for sampling decisions.  But it should be approximately
	 * correct.
	 */
	virtual Color total_emitted_color() const = 0;
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
 * @brief An interface for surface objects that can improve intersection
 * performance by testing many rays at once.
 */
class BreadthSurface: public Object
{
public:
	virtual ~BreadthSurface() {}

	Object::Type get_type() const final {
		return Object::BREADTH_SURFACE;
	}

	/**
	 * @brief Tests a batch of rays against the surface.
	 */
	virtual void intersect_rays(const std::vector<Transform>& parent_xforms, Ray* ray_begin, Ray* ray_end, Intersection *intersections, Stack* data_stack) = 0;
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
	virtual std::unique_ptr<DiceableSurface> copy() const = 0;

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
	virtual std::shared_ptr<MicroSurface> dice(size_t subdivisions) const = 0;
};

#endif // OBJECT_HPP
