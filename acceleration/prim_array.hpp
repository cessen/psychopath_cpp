#ifndef PRIM_ARRAY_HPP
#define PRIM_ARRAY_HPP

#include "numtype.h"

#include <vector>
#include "primitive.hpp"
#include "aggregate.hpp"
#include "ray.hpp"
#include "bbox.hpp"


/*
 * The simplest aggregate.  Just a list of primitives.
 */
class PrimArray: public Aggregate
{
private:
	BBox bbox;
	std::vector<Primitive *> children;

public:
	virtual ~PrimArray();

	// Inherited
	virtual void add_primitives(std::vector<Primitive *> &primitives);
	virtual bool finalize();

	virtual BBox &bounds();
	virtual bool intersect_ray(Ray &ray, Intersection *intersection=NULL);
};




#endif
