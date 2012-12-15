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
	virtual uint32 get_potential_intersections(const Ray &ray, uint32 max_potential, uint_i *ids, uint64 *state);
	virtual Primitive &get_primitive(uint_i id);

	virtual BBoxT &bounds();
	virtual bool intersect_ray(Ray &ray, Intersection *intersection=NULL);
};




#endif
