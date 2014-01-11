#ifndef COLLECTION_HPP
#define COLLECTION_HPP

#include "numtype.h"

#include <vector>
#include <memory>


/**
 * @brief A collection, or set, of primitives.
 *
 * Can itself be transparently treated as a primitive, and therefore must
 * be traceable and forward the tracing on to the appropriate child
 * primtives.
 */
class Collection
{
public:
	virtual ~Collection() {}

	/**
	 * @brief Adds the given primitives to the collection.
	 *
	 * Note that the Collection does not own the added primitives (even though
	 * it takes a pointer to a vector of unique_ptr's) taking a vector of
	 * unique_ptr's), and will not free them.  Their memory needs to be
	 * managed elsewhere.
	 *
	 * Can be called multiple times to add subsequent primitives.
	 * Should NOT be called externally after finalization.
	 */
	virtual void add_primitives(std::vector<std::unique_ptr<Primitive>>* primitives) = 0;

	/**
	 * @brief Does any work necessary before the collection can be traced.
	 *
	 * For example, constructing data structures for more efficient
	 * traversal of its children.
	 * No additional external calls should be be made to add_primitive()
	 * after this is called.
	 */
	virtual bool finalize() = 0;

	/**
	 * @brief Returns the largest primitive ID currently
	 * in the collection.
	 *
	 * Note: this is different than the field "id" in the Primitive class.
	 * This is a collection-specific id that each primitive gets.
	 */
	virtual size_t max_primitive_id() const = 0;

	/**
	 * @brief Fetches a primitive based on id.
	 *
	 * Note: this is different than the field "id" in the Primitive class.
	 * This is a collection-specific id that each primitive gets.
	 */
	virtual Primitive &get_primitive(size_t id) = 0;

	/**
	 * @brief Returns the number of bytes used to store per-ray traversal state.
	 */
	virtual size_t ray_state_size() = 0;

	/**
	 * Retrieves ids of primitives that potentially intersect with a ray.
	 * The number of results is bounded by max_potential.
	 *
	 * @param ray The ray.
	 * @param max_potential The maximum number of results.
	 * @param ids Output parameter, should be an array of ids at least as large as max_potential.
	 * @param state Input/output parameter.  Should point to memory large enough to store the traversal
	                state of the ray.  nullptr just gets the first N potentially
	 *                intersecting primitives.  Array values of all zeros starts as default.
	 *
	 * @returns The number of results acquired.  If zero, that means there were no potential intersections.
	 */
	virtual uint get_potential_intersections(const Ray &ray, float tmax, uint max_potential, size_t *ids, void *state) = 0;



};

#endif
