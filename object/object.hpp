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
#include "surface_shader.hpp"


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
	    COMPLEX_SURFACE,
	    PATCH_SURFACE,
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
	 *
	 * TODO: remove this function!  This is NOT where this should be handled.
	 * This needs to be handled at a point where the material of the object
	 * is known.
	 */
	virtual Color total_emitted_color() const = 0;
};


/**
 * @brief An interface for traditional surface objects that can be easily
 * directly tested against a single ray at a time.
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
 * @brief An interface for surfaces that require more complex handling
 * and which require fast scratch memory.
 */
class ComplexSurface: public Object
{
public:
	virtual ~ComplexSurface() {}

	Object::Type get_type() const final {
		return Object::COMPLEX_SURFACE;
	}

	/**
	 * @brief Tests a batch of rays against the surface.
	 */
	virtual void intersect_rays(const Ray* rays_begin, const Ray* rays_end,
	                            Intersection *intersections,
	                            const Range<const Transform*> parent_xforms,
	                            Stack* data_stack,
	                            const SurfaceShader* surface_shader,
	                            const InstanceID& element_id) const = 0;
};


/**
 * @brief An interface for surface patches with inherent UV coordinates, and
 * which can be easily recursively split into smaller patches.
 *
 * Other than defining get_type() there are no methods defined in this class.
 * However, subclasses of this must nevertheless adhere to an interface and
 * provide certain static methods that certain templated functions end up
 * using.  C++14 and earlier are, unfortunately, not able to describe such
 * interfaces.  Hopefully Concepts Lite in C++17 will allow this.  In the mean
 * time, look at the Bilinear and Bicubic classes for examples of the required
 * interface.
 */
class PatchSurface: public Object
{
public:
	virtual ~PatchSurface() {}

	Object::Type get_type() const final {
		return Object::PATCH_SURFACE;
	}
};

#endif // OBJECT_HPP
