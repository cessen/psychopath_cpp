#ifndef ACCEL_HPP
#define ACCEL_HPP

#include "numtype.h"
#include "ray.hpp"
#include "object.hpp"
#include "bbox.hpp"

#include <vector>
#include <tuple>
#include <memory>

// Forward declaration of Assembly from scene/assembly.hpp
class Assembly;

/**
 * @brief An acceleration structure for a scene hierarchy.
 *
 * This pure virtual class should never be used directly.  It's only purpose
 * is to enforce an interface for classes that inherit from it.
 */
class Accel {
public:
	virtual ~Accel() {}

	/**
	 * @brief Builds the acceleration structure from the given assembly.
	 */
	virtual void build(const Assembly& assembly) = 0;

	/**
	 * @brief Returns the spatial bounds of the acceleration structure.
	 *
	 * Should not be called until after build() is called.
	 */
	virtual const std::vector<BBox>& bounds() const = 0;
};


/**
 * @brief An acceleration structure traverser that traverses with many rays at once
 * in a breadth-first fashion.
 *
 * This pure virtual template class should never be used directly.  It's only purpose
 * is to enforce an interface for classes that inherit from it.
 */
template <typename T>
class AccelStreamTraverser {
public:
	virtual ~AccelStreamTraverser() {}

	/**
	 * @brief Initializes the traverser for traversing the given
	 * acceleration structure.
	 */
	virtual void init_accel(const T& accel) = 0;

	/**
	 * @brief Initializes the traverser for traversing with
	 * the given WorldRays.
	 *
	 * This resets any traversal already in progress.
	 */
	virtual void init_rays(Ray* begin, Ray* end) = 0;

	/**
	 * @brief Traverses to the next relevant object.
	 *
	 * Returns a tuple with a pair of iterators to the begin and end of the
	 * relevant Rays, and an index to the object instance they need to be
	 * tested against.
	 *
	 * When traversal is complete, begin == end and object == 0.
	 */
	virtual std::tuple<Ray*, Ray*, size_t>
	next_object() = 0;
};

#endif // ACCEL_HPP
