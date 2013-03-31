#ifndef PRIM_ARRAY_HPP
#define PRIM_ARRAY_HPP

#include "numtype.h"

#include <vector>
#include "primitive.hpp"
#include "collection.hpp"
#include "ray.hpp"
#include "bbox.hpp"


/*
 * The simplest aggregate.  Just a list of primitives.
 */
class PrimArray: public Collection
{
private:
	BBoxT bbox;
	std::vector<Primitive *> children;

public:
	virtual ~PrimArray();

	// Inherited
	virtual void add_primitives(std::vector<Primitive *> &primitives);
	virtual bool finalize();
	virtual uint_i max_primitive_id() const;
	virtual uint get_potential_intersections(const Ray &ray, uint max_potential, uint_i *ids, void *state);
	virtual Primitive &get_primitive(uint_i id);
	virtual uint_i size() {
		return children.size();
	}
	virtual size_t ray_state_size() {
		return 8;
	}

	virtual BBoxT &bounds();
	virtual bool intersect_ray(const Ray &ray, Intersection *intersection=nullptr);
};




#endif
