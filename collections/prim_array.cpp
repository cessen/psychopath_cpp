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

uint_i PrimArray::max_primitive_id() const
{
	return children.size();
}

Primitive &PrimArray::get_primitive(uint_i id)
{
	return *(children[id]);
}

BBoxT &PrimArray::bounds()
{
	return bbox;
}

bool PrimArray::intersect_ray(const Ray &ray, Intersection *intersection)
{
	std::vector<Primitive *> temp_prim;
	float32 tnear, tfar;
	bool hit = false;
	int32 size = children.size();

	for (int32 i=0; i < size; i++) {
		if (children[i]->bounds().intersect_ray(ray, &tnear, &tfar)) {
			// Trace!
			hit |= children[i]->intersect_ray(ray, intersection);

			// Early out for shadow rays
			if (hit && ray.is_shadow_ray)
				break;
		}
	}

	return hit;
}


uint PrimArray::get_potential_intersections(const Ray &ray, uint max_potential, uint_i *ids, void *state)
{
	const uint32 size = children.size();
	float32 tnear, tfar;

	// Fetch starting index
	uint32 i = 0;
	if (state != nullptr)
		i = *((uint64 *)state);

	// Accumulate potential primitive intersections
	uint32 hits_so_far = 0;
	for (; i < size && hits_so_far < max_potential; i++) {
		if (children[i]->bounds().intersect_ray(ray, &tnear, &tfar)) {
			ids[hits_so_far] = i;
			hits_so_far++;
		}
	}

	// Write last index
	if (state != nullptr)
		*((uint64 *)state) = i;

	return hits_so_far;
}
