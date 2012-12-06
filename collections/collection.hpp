#ifndef COLLECTION_HPP
#define COLLECTION_HPP

#include "numtype.h"

#include <vector>


/**
 * An aggregate, or set, of primitives.
 * Can itself be transparently treated as a primitive, and therefore must
 * be traceable and forward the tracing on to the appropriate child
 * primtives.
 */
class Collection: public Primitive
{
public:
	virtual ~Collection() {}

	/**
	 * Adds the given primitives to the aggregate.
	 * Can be called multiple times to add subsequent primitives.
	 * Should NOT be called externally after finalization.
	 */
	virtual void add_primitives(std::vector<Primitive *> &primitives) = 0;

	/**
	 * Does any work necessary before the aggregate can be traced.
	 * For example, constructing data structures for more efficient
	 * traversal of its children.
	 * No additional external calls should be be made to add_primitive()
	 * after this is called.
	 */
	virtual bool finalize() = 0;

	/**
	 * Fetches a primitive based on id.
	 */
	virtual Primitive &get_primitive(uint64 id) = 0;

	/**
	 * Retrieves ids of primitives that potentially intersect with a ray.
	 * The number of results is bounded by max_potential.
	 *
	 * @param ray The ray.
	 * @param max_potential The maximum number of results.
	 * @param ids Output parameter, should be an array of ids at least as large as max_potential.
	 * @param restart Input/output parameter.  Should point to an array of two 64-bit unsigned ints.
	 *                Encodes information about the traversal state.  NULL just gets the first N potentially
	 *                intersecting primitives.  Array values of all zeros starts as default.
	 *
	 * @returns The number of results acquired.  If zero, that means there were no potential intersections.
	 */
	virtual uint32 get_potential_intersections(Ray ray, uint32 max_potential, uint32 *ids, uint64 *restart) = 0;



};

#endif
