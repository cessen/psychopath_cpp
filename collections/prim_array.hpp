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
	virtual size_t max_primitive_id() const;
	virtual uint get_potential_intersections(const Ray &ray, float tmax, uint max_potential, size_t *ids, void *state);
	virtual Primitive &get_primitive(size_t id);
	virtual size_t size() {
		return children.size();
	}
	virtual size_t ray_state_size() {
		return 8;
	}

	virtual BBoxT &bounds();
	virtual bool intersect_ray(const Ray &ray, Intersection *intersection=nullptr);
};




#endif
