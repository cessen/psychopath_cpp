#include "numtype.h"

#include <iostream>
#include "ray.hpp"
#include "prim_array.hpp"



PrimArray::~PrimArray()
{
	int32 size = children.size();

	for (int32 i=0; i < size; i++) {
		delete children[i];
	}
}

void PrimArray::add_primitives(std::vector<Primitive *> &primitives)
{
	int32 start = children.size();
	int32 added = primitives.size();
	children.resize(start + added);

	for (int32 i=0; i < added; i++) {
		children[start + i] = primitives[i];
	}
}

bool PrimArray::finalize()
{
	// Initialize primitive's bounding boxes
	uint32 s = children.size();
	for (uint32 i = 0; i < s; i++) {
		children[i]->bounds();
	}

	return true;
}

Primitive &PrimArray::get_primitive(uint64 id)
{
	return *(children[id]);
}

BBoxT &PrimArray::bounds()
{
	return bbox;
}

bool PrimArray::intersect_ray(Ray &ray, Intersection *intersection)
{
	std::vector<Primitive *> temp_prim;
	float32 tnear, tfar;
	bool hit = false;
	int32 size = children.size();

	for (int32 i=0; i < size; i++) {
		if (children[i]->bounds().intersect_ray(ray, &tnear, &tfar)) {
			if (children[i]->is_traceable(ray.min_width(tnear, tfar))) {
				// Trace!
				hit |= children[i]->intersect_ray(ray, intersection);

				// Early out for shadow rays
				if (hit && ray.is_shadow_ray)
					break;
			} else {
				// Split!
				children[i]->refine(temp_prim);
				delete children[i];
				children[i] = temp_prim[0];

				int32 first = size;
				size += (temp_prim.size() - 1);
				children.resize(size);

				for (int32 j=first; j < size; j++) {
					children[j] = temp_prim[1+j-first];
				}
				i--;
			}
		}
	}

	return hit;
}


uint32 PrimArray::get_potential_intersections(Ray ray, uint32 max_potential, uint32 *ids, uint64 *restart)
{
	const uint32 size = children.size();
	float32 tnear, tfar;

	// Fetch starting index
	uint32 i = 0;
	if (restart != NULL)
		i = restart[0];

	// Accumulate potential primitive intersections
	uint32 hits_so_far = 0;
	for (; i < size && hits_so_far < max_potential; i++) {
		if (children[i]->bounds().intersect_ray(ray, &tnear, &tfar)) {
			ids[hits_so_far] = i;
			hits_so_far++;
		}
	}

	// Write last index
	if (restart != NULL)
		restart[0] = i;

	return hits_so_far;
}
