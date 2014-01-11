#include "numtype.h"

#include <iostream>
#include <algorithm>
#include <memory>
#include "ray.hpp"
#include "bvh.hpp"
#include <cmath>


#define IS_LEAF 1

void BVH::add_primitives(std::vector<std::unique_ptr<Primitive>>* primitives)
{
	size_t start = bag.size();
	size_t added = primitives->size();
	bag.reserve(start + added);

	for (auto& p: *primitives)
		bag.push_back(BVHPrimitive(p.get()));
}

bool BVH::finalize()
{
	if (bag.size() == 0)
		return true;
	recursive_build(0, 0, bag.size()-1);
	bag.resize(0);
	return true;
}

size_t BVH::max_primitive_id() const
{
	return nodes.size();
}

// TODO: should be changed to fetch based on primitive id, not node id.
Primitive &BVH::get_primitive(size_t id)
{
	return *(nodes[id].data);
}


struct CompareToMid {
	int32_t dim;
	float mid;

	CompareToMid(int32_t d, float m) {
		dim = d;
		mid = m;
	}

	bool operator()(BVHPrimitive &a) const {
		return a.c[dim] < mid;
	}
};

struct CompareDim {
	int32_t dim;

	CompareDim(int32_t d) {
		dim = d;
	}

	bool operator()(BVHPrimitive &a, BVHPrimitive &b) const {
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
uint32_t BVH::split_primitives(size_t first_prim, size_t last_prim)
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
	BVHPrimitive *mid_ptr = std::partition(&bag[first_prim],
	                                       (&bag[last_prim])+1,
	                                       CompareToMid(max_axis, pmid));

	uint32_t split = (mid_ptr - &bag.front());
	if (split > 0)
		split--;

	if (split < first_prim)
		split = first_prim;

	return split;
}

#else

/* SAH-based split */
uint32_t BVH::split_primitives(size_t first_prim, size_t last_prim)
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


			cost_x[i] = (b0_x.surface_area() / log2(count0_x)) + (b1_x.surface_area() / log2(count1_x));
			cost_y[i] = (b0_y.surface_area() / log2(count0_y)) + (b1_y.surface_area() / log2(count1_y));
			cost_z[i] = (b0_z.surface_area() / log2(count0_z)) + (b1_z.surface_area() / log2(count1_z));
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
		nodes[me].data = bag[first_prim].data;

		// Copy bounding boxes
		nodes[me].bbox_index = bboxes.size();
		nodes[me].ts = bag[first_prim].data->bounds().size();
		for (size_t i = 0; i < nodes[me].ts; i++)
			bboxes.push_back(bag[first_prim].data->bounds()[i]);
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


uint BVH::get_potential_intersections(const Ray &ray, float tmax, uint max_potential, size_t *ids, void *state)
{
	// Algorithm is the BVH2 algorithm from the paper
	// "Stackless Multi-BVH Traversal for CPU, MIC and GPU Ray Tracing"
	// by Afra et al.

	// Get state
	uint64_t& node = static_cast<uint64_t *>(state)[0];
	uint64_t& bit_stack = static_cast<uint64_t *>(state)[1];

	// Check if it's an empty BVH or if we have the "finished" magic number
	if (nodes.size() == 0 || node == ~uint64_t(0))
		return 0;

	const Vec3 inv_d = ray.get_inverse_d();
	const std::array<uint32_t, 3> d_is_neg = ray.get_d_is_neg();

	// Traverse the BVH
	uint32_t hits_so_far = 0;
	float hitt0a, hitt1a;
	float hitt0b, hitt1b;
	while (hits_so_far < max_potential) {
		const Node& n = nodes[node];

		if (n.flags & IS_LEAF) {
			ids[hits_so_far++] = node;
		} else {
			bool hit0, hit1;
			hitt0a = hitt1a = hitt0b = hitt1b = std::numeric_limits<float>::infinity();
			hit0 = intersect_node(child1(node), ray, inv_d, d_is_neg, &hitt0a, &hitt1a) && hitt0a < tmax;
			hit1 = intersect_node(child2(node), ray, inv_d, d_is_neg, &hitt0b, &hitt1b) && hitt0b < tmax;

			if (hit0 || hit1) {
				bit_stack <<= 1;
				if (hitt0a < hitt0b) {
					node = child1(node);
					if (hit1)
						bit_stack |= 1;
				} else {
					node = child2(node);
					if (hit0)
						bit_stack |= 1;
				}
				continue;
			}
		}

		// If we've completed the full traversal
		if (bit_stack == 0) {
			node = ~uint64_t(0); // Magic number for "finished"
			break;
		}

		// Find the next node to work from
		while ((bit_stack & 1) == 0) {
			node = nodes[node].parent_index;
			bit_stack >>= 1;
		}

		// Go to sibling
		bit_stack &= ~uint64_t(1);
		node = sibling(node);
	}

	// Return the number of primitives accumulated
	return hits_so_far;
}
