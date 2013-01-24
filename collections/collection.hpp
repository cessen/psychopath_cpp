#ifndef COLLECTION_HPP
#define COLLECTION_HPP

#include "numtype.h"

#include <vector>


/**
 * @brief A collection, or set, of primitives.
 *
 * Can itself be transparently treated as a primitive, and therefore must
 * be traceable and forward the tracing on to the appropriate child
 * primtives.
 */
class Collection: public Primitive
{
public:
	virtual ~Collection() {}

	/**
	 * @brief Adds the given primitives to the collection.
	 *
	 * Can be called multiple times to add subsequent primitives.
	 * Should NOT be called externally after finalization.
	 */
	virtual void add_primitives(std::vector<Primitive *> &primitives) = 0;

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
	 * @brief Returns the number of primitives in the collection.
	 *
	 * This should also be the largest possible primitive id, as taken by
	 * e.g. get_primitive().
	 */
	virtual uint_i size() = 0;

	/**
	 * @brief Fetches a primitive based on id.
	 */
	virtual Primitive &get_primitive(uint_i id) = 0;

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
	virtual uint get_potential_intersections(const Ray &ray, uint max_potential, uint_i *ids, void *state) = 0;



};

#endif
