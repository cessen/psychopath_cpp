#ifndef AGGREGATE_HPP
#define AGGREGATE_HPP

#include <vector>

/*
 * An aggregate, or set, of primitives.
 * Can itself be transparently treated as a primitive, and therefore must
 * be traceable and forward the tracing on to the appropriate child
 * primtives.
 */
class Aggregate: public Primitive
{
    public:
        virtual ~Aggregate() {}
    
        /*
         * Adds the given primitives to the aggregate.
         * Can be called multiple times to add subsequent primitives.
         * Should NOT be called externally after finalization.
         */
        virtual void add_primitives(std::vector<Primitive *> &primitives) = 0;
        
        /*
         * Does any work necessary before the aggregate can be traced.
         * For example, constructing data structures for more efficient
         * traversal of its children.
         * No additional external calls should be be made to add_primitive()
         * after this is called.
         */
        virtual bool finalize() = 0;
};

#endif
