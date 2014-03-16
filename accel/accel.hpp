#ifndef ACCEL_HPP
#define ACCEL_HPP

#include "numtype.h"
#include "ray.hpp"
#include "scene_graph.hpp"
#include "object.hpp"

#include <vector>
#include <tuple>
#include <memory>


/**
 * @brief An acceleration structure for a scene hierarchy.
 *
 * This pure virtual class should never be used directly.  It's only purpose
 * is to enforce an interface for classes that inherit from it.
 */
class Accel
{
public:
	virtual ~Accel() {}

	/**
	 * @brief Builds the acceleration structure from the given objects.
	 *
	 * Note that the Accel does not own the passed objects (even though
	 * it takes a pointer to a vector of unique_ptr's), and will not free
	 * them.  Their memory needs to be managed elsewhere (e.g. by the Scene).
	 */
	virtual void build(const SceneGraph& scene_graph) = 0;
};


/**
 * @brief An acceleration structure traverser that traverses with many rays at once
 * in a breadth-first fashion.
 *
 * This pure virtual template class should never be used directly.  It's only purpose
 * is to enforce an interface for classes that inherit from it.
 */
template <typename T>
class AccelStreamTraverser
{
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
	virtual void init_rays(const WorldRay* begin, const WorldRay* end) = 0;

	/**
	 * @brief Traverses to the next relevant object.
	 *
	 * Returns a tuple with a pair of iterators to the begin and end of the relevant
	 * Rays, and a pointer to the object they need to be tested against.
	 *
	 * When traversal is complete, begin == end and object == nullptr.
	 */
	virtual std::tuple<Ray*, Ray*, Object*>
	next_object() = 0;
};

#endif // ACCEL_HPP
