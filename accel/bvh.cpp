#include "numtype.h"

#include <iostream>
#include <algorithm>
#include <memory>
#include <cmath>

#include "bvh.hpp"

#include "utils.hpp"
#include "ray.hpp"
#include "assembly.hpp"

void BVH::build(const Assembly& _assembly)
{
	assembly = &_assembly;

	// Abbreviating subsequent code
	const auto& instances = assembly->instances;

	// Create BVHPrimitive bag
	bag.reserve(instances.size());
	for (size_t i = 0; i < instances.size(); ++i) {
		// Skip if it's a light
		// TODO: lights should be included too, with MIS
		if (assembly->instances[i].type == Instance::OBJECT) {
			Object* obj = assembly->objects[assembly->instances[i].data_index].get();
			if (obj->get_type() == Object::LIGHT)
				continue;
		}

		// Get instance bounds at time 0.5
		BBox bb = assembly->instance_bounds_at(0.5f, i);

		// Create primitive
		BVHPrimitive prim;
		prim.instance_index = i;
		prim.bmin = bb.min;
		prim.bmax = bb.max;
		prim.c = lerp(0.5f, bb.min, bb.max);

		// Add it to the bag
		bag.push_back(prim);
	}

	if (bag.size() == 0)
		return;

	recursive_build(0, 0, bag.size()-1);
	bag.resize(0);
}

bool BVH::finalize()
{
	// TODO: this method is no longer necessary
	return true;
}

struct CompareToMid {
	int32_t dim;
	float mid;

	CompareToMid(int32_t d, float m) {
		dim = d;
		mid = m;
	}

	bool operator()(BVH::BVHPrimitive &a) const {
		return a.c[dim] < mid;
	}
};

struct CompareDim {
	int32_t dim;

	CompareDim(int32_t d) {
		dim = d;
	}

	bool operator()(BVH::BVHPrimitive &a, BVH::BVHPrimitive &b) const {
		return a.c[dim] < b.c[dim];
	}
};



#if 1
/*
 * Determines the split of the primitives in bag starting
 * at first and ending at last.  May reorder that section of the
 * list.  Used in recursive_build for BVH construction.
 * Returns the split index (last index of the first group).
 *
 * TODO: SAH splitting seems to be very buggy.  Fix.
 */
size_t BVH::split_primitives(size_t first_prim, size_t last_prim)
{
	// Find the minimum and maximum centroid values on each axis
	Vec3 min, max;
	min = bag[first_prim].c;
	max = bag[first_prim].c;
	for (uint32_t i = first_prim+1; i <= last_prim; i++) {
		for (int32_t d = 0; d < 3; d++) {
			min[d] = min[d] < bag[i].c[d] ? min[d] : bag[i].c[d];
			max[d] = max[d] > bag[i].c[d] ? max[d] : bag[i].c[d];
		}
	}

	// Find the axis with the maximum extent
	int32_t max_axis = 0;
	if ((max.y - min.y) > (max.x - min.x))
		max_axis = 1;
	if ((max.z - min.z) > (max.y - min.y))
		max_axis = 2;

	// Sort and split the list
	float pmid = .5f * (min[max_axis] + max[max_axis]);
	auto mid_itr = std::partition(bag.begin()+first_prim,
	                              bag.begin()+last_prim+1,
	                              CompareToMid(max_axis, pmid));

	size_t split = std::distance(bag.begin(), mid_itr);
	if (split > 0)
		split--;

	if (split < first_prim)
		split = first_prim;

	return split;
}

#else

/* SAH-based split */
size_t BVH::split_primitives(size_t first_prim, size_t last_prim)
{
	uint32_t split;

	// Find the minimum and maximum centroid values on each axis
	Vec3 min, max;
	min = bag[first_prim].c;
	max = bag[first_prim].c;
	for (uint32_t i = first_prim+1; i <= last_prim; i++) {
		for (int32_t d = 0; d < 3; d++) {
			min[d] = min[d] < bag[i].c[d] ? min[d] : bag[i].c[d];
			max[d] = max[d] > bag[i].c[d] ? max[d] : bag[i].c[d];
		}
	}

	const int32_t nBuckets = 12;
	if ((last_prim - first_prim) <= 4) {
		// No need to do SAH-based split

		// Find the axis with the maximum extent
		int32_t max_axis = 0;
		if ((max.y - min.y) > (max.x - min.x))
			max_axis = 1;
		if ((max.z - min.z) > (max.y - min.y))
			max_axis = 2;

		split = (first_prim + last_prim) / 2;

		std::partial_sort(&bag[first_prim],
		                  (&bag[split])+1,
		                  (&bag[last_prim])+1, CompareDim(max_axis));
	} else {
		// SAH-based split

		// Initialize buckets
		BucketInfo buckets_x[nBuckets];
		BucketInfo buckets_y[nBuckets];
		BucketInfo buckets_z[nBuckets];
		for (uint32_t i = first_prim; i <= last_prim; i++) {
			int32_t b_x = 0;
			int32_t b_y = 0;
			int32_t b_z = 0;

			if (max[0] > min[0])
				b_x = nBuckets * ((bag[i].c[0] - min[0]) / (max[0] - min[0]));
			if (max[1] > min[1])
				b_y = nBuckets * ((bag[i].c[1] - min[1]) / (max[1] - min[1]));
			if (max[2] > min[2])
				b_z = nBuckets * ((bag[i].c[2] - min[2]) / (max[2] - min[2]));

			if (b_x == nBuckets)
				b_x = nBuckets-1;
			if (b_y == nBuckets)
				b_y = nBuckets-1;
			if (b_z == nBuckets)
				b_z = nBuckets-1;

			// Increment count on the bucket, and merge bounds
			buckets_x[b_x].count++;
			buckets_y[b_y].count++;
			buckets_z[b_z].count++;
			for (int32_t j=0; j < 3; j++) {
				buckets_x[b_x].bb[0].min[j] = bag[i].bmin[j] < buckets_x[b_x].bb[0].min[j] ? bag[i].bmin[j] : buckets_x[b_x].bb[0].min[j];
				buckets_x[b_x].bb[0].max[j] = bag[i].bmax[j] > buckets_x[b_x].bb[0].max[j] ? bag[i].bmax[j] : buckets_x[b_x].bb[0].max[j];

				buckets_y[b_y].bb[0].min[j] = bag[i].bmin[j] < buckets_y[b_y].bb[0].min[j] ? bag[i].bmin[j] : buckets_y[b_y].bb[0].min[j];
				buckets_y[b_y].bb[0].max[j] = bag[i].bmax[j] > buckets_y[b_y].bb[0].max[j] ? bag[i].bmax[j] : buckets_y[b_y].bb[0].max[j];

				buckets_z[b_z].bb[0].min[j] = bag[i].bmin[j] < buckets_z[b_z].bb[0].min[j] ? bag[i].bmin[j] : buckets_z[b_z].bb[0].min[j];
				buckets_z[b_z].bb[0].max[j] = bag[i].bmax[j] > buckets_z[b_z].bb[0].max[j] ? bag[i].bmax[j] : buckets_z[b_z].bb[0].max[j];
			}
		}

		// Calculate the cost of each split
		float cost_x[nBuckets-1];
		float cost_y[nBuckets-1];
		float cost_z[nBuckets-1];
		for (int32_t i = 0; i < nBuckets-1; ++i) {
			BBoxT b0_x, b1_x;
			BBoxT b0_y, b1_y;
			BBoxT b0_z, b1_z;
			int32_t count0_x = 0, count1_x = 0;
			int32_t count0_y = 0, count1_y = 0;
			int32_t count0_z = 0, count1_z = 0;

			b0_x.copy(buckets_x[0].bb);
			b0_y.copy(buckets_y[0].bb);
			b0_z.copy(buckets_z[0].bb);
			for (int32_t j = 0; j <= i; ++j) {
				b0_x.merge_with(buckets_x[j].bb);
				count0_x += buckets_x[j].count;

				b0_y.merge_with(buckets_y[j].bb);
				count0_y += buckets_y[j].count;

				b0_z.merge_with(buckets_z[j].bb);
				count0_z += buckets_z[j].count;
			}

			b1_x.copy(buckets_x[i+1].bb);
			b1_y.copy(buckets_y[i+1].bb);
			b1_z.copy(buckets_z[i+1].bb);
			for (int32_t j = i+1; j < nBuckets; ++j) {
				b1_x.merge_with(buckets_x[j].bb);
				count1_x += buckets_x[j].count;

				b1_y.merge_with(buckets_y[j].bb);
				count1_y += buckets_y[j].count;

				b1_z.merge_with(buckets_z[j].bb);
				count1_z += buckets_z[j].count;
			}


			cost_x[i] = (b0_x.surface_area() / fastlog2(count0_x)) + (b1_x.surface_area() / fastlog2(count1_x));
			cost_y[i] = (b0_y.surface_area() / fastlog2(count0_y)) + (b1_y.surface_area() / fastlog2(count1_y));
			cost_z[i] = (b0_z.surface_area() / fastlog2(count0_z)) + (b1_z.surface_area() / fastlog2(count1_z));
		}

		// Find the most efficient split
		float minCost = cost_x[0];
		int32_t split_axis = 0;
		uint32_t minCostSplit = 0;
		// X
		for (int32_t i = 1; i < nBuckets-1; ++i) {
			if (cost_x[i] < minCost) {
				minCost = cost_x[i];
				minCostSplit = i;
				split_axis = 0;
			}

			if (cost_y[i] < minCost) {
				minCost = cost_y[i];
				minCostSplit = i;
				split_axis = 1;
			}

			if (cost_z[i] < minCost) {
				minCost = cost_z[i];
				minCostSplit = i;
				split_axis = 2;
			}
		}

		float pmid = min[split_axis] + (((max[split_axis] - min[split_axis]) / nBuckets) * (minCostSplit+1));
		BVHPrimitive *mid_ptr = std::partition(&bag[first_prim],
		                                       (&bag[last_prim])+1,
		                                       CompareToMid(split_axis, pmid));

		split = (mid_ptr - &bag.front());
	}

	if (split > 0)
		split--;

	if (split < first_prim)
		split = first_prim;

	return split;
}
#endif


/*
 * Recursively builds the BVH starting at the given node with the given
 * first and last primitive indices (in bag).
 */
size_t BVH::recursive_build(size_t parent, size_t first_prim, size_t last_prim)
{
	// Allocate the node
	const size_t me = nodes.size();
	nodes.push_back(Node());


	nodes[me].flags = 0;
	nodes[me].parent_index = parent;

	if (first_prim == last_prim) {
		// Leaf node
		nodes[me].flags |= IS_LEAF;
		nodes[me].data_index = bag[first_prim].instance_index;

		// Copy bounding boxes
		auto bbs = assembly->instance_bounds(bag[first_prim].instance_index);
		nodes[me].bbox_index = bboxes.size();
		nodes[me].ts = bbs.size();
		// Append bbs to bboxes
		bboxes.insert(bboxes.end(), bbs.begin(), bbs.end());
	} else {
		// Not a leaf node

		// Create child nodes
		uint32_t split_index = split_primitives(first_prim, last_prim);
		const size_t child1i = recursive_build(me, first_prim, split_index);
		const size_t child2i = recursive_build(me, split_index+1, last_prim);

		nodes[me].child_index = child2i;


		// Calculate bounds
		nodes[me].bbox_index = bboxes.size();
		// If both children have same number of time samples
		if (nodes[child1i].ts == nodes[child2i].ts) {
			nodes[me].ts = nodes[child1i].ts;

			// Copy merged bounding boxes
			for (size_t i = 0; i < nodes[me].ts; i++) {
				bboxes.push_back(bboxes[nodes[child1i].bbox_index+i]);
				bboxes.back().merge_with(bboxes[nodes[child2i].bbox_index+i]);
			}

		}
		// If children have different number of time samples
		else {
			nodes[me].ts = 1;

			// Merge children's bboxes to get our bbox
			bboxes.push_back(bboxes[nodes[child1i].bbox_index]);
			for (size_t i = 1; i < nodes[child1i].ts; i++)
				bboxes.back().merge_with(bboxes[nodes[child1i].bbox_index+i]);
			for (size_t i = 0; i < nodes[child2i].ts; i++)
				bboxes.back().merge_with(bboxes[nodes[child2i].bbox_index+i]);
		}
	}

	return me;
}


std::tuple<Ray*, Ray*, size_t> BVHStreamTraverser::next_object()
{
	// If there aren't any objects in the scene, return finished
	if (bvh->nodes.size() == 0)
		return std::make_tuple(rays_end, rays_end, 0);

	float near_t, far_t;

	while (stack_ptr >= 0) {
		// Test rays against current node
		int hit_count = 0;
		for (auto itr = ray_stack[stack_ptr].first; itr < ray_stack[stack_ptr].second; ++itr) {
			const bool hit = bvh->intersect_node(node_stack[stack_ptr], *itr, &near_t, &far_t);

			if (hit) {
				++hit_count;
				itr->flags |= Ray::TRAV_HIT;
			} else {
				itr->flags &= ~Ray::TRAV_HIT;
			}
		}

		if (hit_count > 0) {
			// Calculate what percentage of the rays hit the node
			const float hit_ratio = static_cast<float>(hit_count) / static_cast<float>(std::distance(ray_stack[stack_ptr].first, ray_stack[stack_ptr].second));

			// If it's worth it, partition rays into rays that hit and didn't hit
			if (hit_ratio < 0.9f) {
				ray_stack[stack_ptr].first = std::partition(ray_stack[stack_ptr].first, ray_stack[stack_ptr].second, [this](const Ray& r) {
					return ((r.flags & Ray::TRAV_HIT) == 0) || ((r.flags & Ray::DONE) != 0);
				});
			}
		}

		// If none of the rays hit
		if (hit_count == 0) {
			--stack_ptr;
		}
		// If it's a leaf
		else if (bvh->is_leaf(node_stack[stack_ptr])) {
			auto rv = std::make_tuple(&(*ray_stack[stack_ptr].first), &(*ray_stack[stack_ptr].second), bvh->nodes[node_stack[stack_ptr]].data_index);
			--stack_ptr;
			return rv;
		}
		// Traverse deeper
		else {
			ray_stack[stack_ptr+1] = ray_stack[stack_ptr];

			node_stack[stack_ptr+1] = bvh->child1(node_stack[stack_ptr]);
			node_stack[stack_ptr] = bvh->child2(node_stack[stack_ptr]);

			stack_ptr++;
		}
	}

	// Finished traversal
	return std::make_tuple(rays_end, rays_end, 0);
}
