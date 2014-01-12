#include "numtype.h"

#include <iostream>
#include <algorithm>
#include <memory>
#include <tuple>
#include <iterator>
#include "simd.hpp"
#include "ray.hpp"
#include "bvh2.hpp"
#include <cmath>


#define IS_LEAF 1<<0
#define IS_RIGHT 1<<1

void BVH2::add_primitives(std::vector<std::unique_ptr<Primitive>>* primitives)
{
	for (auto& p: *primitives)
		prim_bag.push_back(BuildPrimitive(p.get()));
}

bool BVH2::finalize()
{
	if (prim_bag.size() == 0)
		return true;

	// Build BVH
	recursive_build(0, 0, prim_bag.size()-1);

	// Pack BVH into more efficient form
	pack();

	// Empty the temporary build sets
	prim_bag.clear();
	build_nodes.clear();
	build_bboxes.clear();

	return true;
}

size_t BVH2::max_primitive_id() const
{
	return nodes.size();
}

// TODO: should be changed to fetch based on primitive id, not node id.
Primitive &BVH2::get_primitive(size_t id)
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

	bool operator()(BVH2::BuildPrimitive &a) const {
		return a.c[dim] < mid;
	}
};

struct CompareDim {
	int32_t dim;

	CompareDim(int32_t d) {
		dim = d;
	}

	bool operator()(BVH2::BuildPrimitive &a, BVH2::BuildPrimitive &b) const {
		return a.c[dim] < b.c[dim];
	}
};


/*
 * Determines the split of the primitives in bag starting
 * at first and ending at last.  May reorder that section of the
 * list.  Used in recursive_build for BVH construction.
 * Returns the split index (last index of the first group).
 */
size_t BVH2::split_primitives(size_t first_prim, size_t last_prim)
{
	// Find the minimum and maximum centroid values on each axis
	Vec3 min, max;
	min = prim_bag[first_prim].c;
	max = prim_bag[first_prim].c;
	for (uint32_t i = first_prim+1; i <= last_prim; i++) {
		for (int32_t d = 0; d < 3; d++) {
			min[d] = min[d] < prim_bag[i].c[d] ? min[d] : prim_bag[i].c[d];
			max[d] = max[d] > prim_bag[i].c[d] ? max[d] : prim_bag[i].c[d];
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
	auto mid_itr = std::partition(prim_bag.begin()+first_prim,
	                              prim_bag.begin()+last_prim+1,
	                              CompareToMid(max_axis, pmid));

	size_t split = std::distance(prim_bag.begin(), mid_itr);
	if (split > 0)
		split--;

	if (split < first_prim)
		split = first_prim;

	return split;
}


/*
 * Recursively builds the BVH starting at the given node with the given
 * first and last primitive indices (in bag).
 */
size_t BVH2::recursive_build(size_t parent, size_t first_prim, size_t last_prim)
{
	// Allocate the node
	const size_t me = build_nodes.size();
	build_nodes.push_back(BuildNode());


	build_nodes[me].flags = 0;
	build_nodes[me].parent_index = parent;

	if (first_prim == last_prim) {
		// Leaf node

		build_nodes[me].flags |= IS_LEAF;
		build_nodes[me].data = prim_bag[first_prim].data;

		// Copy bounding boxes
		build_nodes[me].bbox_index = build_bboxes.size();
		build_nodes[me].ts = prim_bag[first_prim].data->bounds().size();
		for (size_t i = 0; i < build_nodes[me].ts; i++)
			build_bboxes.push_back(prim_bag[first_prim].data->bounds()[i]);
	} else {
		// Not a leaf node

		// Create child nodes
		uint32_t split_index = split_primitives(first_prim, last_prim);
		const size_t child1i = recursive_build(me, first_prim, split_index);
		const size_t child2i = recursive_build(me, split_index+1, last_prim);

		build_nodes[me].child_index = child2i;


		// Calculate bounds
		build_nodes[me].bbox_index = build_bboxes.size();
		// If both children have same number of time samples
		if (build_nodes[child1i].ts == build_nodes[child2i].ts) {
			build_nodes[me].ts = build_nodes[child1i].ts;

			// Copy merged bounding boxes
			for (size_t i = 0; i < build_nodes[me].ts; i++) {
				build_bboxes.push_back(build_bboxes[build_nodes[child1i].bbox_index+i]);
				build_bboxes.back().merge_with(build_bboxes[build_nodes[child2i].bbox_index+i]);
			}

		}
		// If children have different number of time samples
		else {
			build_nodes[me].ts = 1;

			// Merge children's bboxes to get our bbox
			build_bboxes.push_back(build_bboxes[build_nodes[child1i].bbox_index]);
			for (size_t i = 1; i < build_nodes[child1i].ts; i++)
				build_bboxes.back().merge_with(build_bboxes[build_nodes[child1i].bbox_index+i]);
			for (size_t i = 0; i < build_nodes[child2i].ts; i++)
				build_bboxes.back().merge_with(build_bboxes[build_nodes[child2i].bbox_index+i]);
		}
	}

	return me;
}


// Packs the BVH into a more efficient form
void BVH2::pack()
{
	if (build_nodes.size() == 0)
		return;

	nodes.push_back(Node());

	for (size_t bni = 0; bni < build_nodes.size(); ++bni) {
		BuildNode& bn = build_nodes[bni];
		size_t ni = nodes.size() - 1; // Node index

		// Set the values that don't depend on whether this
		// is a leaf node or not.
		nodes[ni].parent_index = bn.parent_index;
		if (bn.flags & IS_RIGHT)
			nodes[bn.parent_index].child_index = ni;  // Set parent's child_index field to point to this

		// Set the values that _do_ depend on whether this is
		// a leaf node or not.
		if (bn.flags & IS_LEAF) {
			nodes[ni].child_index = 0; // Indicates that this is a leaf node
			nodes[ni].data = bn.data;
			nodes.push_back(Node());
		} else {
			BuildNode& child1 = build_nodes[bni+1];
			BuildNode& child2 = build_nodes[bn.child_index];

			// Let right child know that it's right
			child2.flags |= IS_RIGHT;

			// Set the parent index fields in the child build nodes
			// to refer to the parent Node instead of the parent BuildNode
			child1.parent_index = ni;
			child2.parent_index = ni;

			// If children have same number of time samples, easy
			if (child1.ts == child2.ts) {
				nodes[ni].time_samples = child1.ts;
				for (uint16_t i = 0; i < nodes[ni].time_samples; ++i) {
					nodes.back().bounds = BBox2(build_bboxes[child1.bbox_index+i], build_bboxes[child2.bbox_index+i]);
					nodes.push_back(Node());
				}
			}
			// If child have different number of time samples,
			// merge time samples into a single sample
			else {
				nodes[ni].time_samples = 1;
				BBox b1, b2;
				for (uint16_t i = 0; i < child1.ts; ++i)
					b1.merge_with(build_bboxes[child1.bbox_index+i]);
				for (uint16_t i = 0; i < child2.ts; ++i)
					b2.merge_with(build_bboxes[child2.bbox_index+i]);

				nodes[ni].bounds = BBox2(b1, b2);

				nodes.push_back(Node());
			}
		}
	}
}


uint BVH2::get_potential_intersections(const Ray &ray, float tmax, uint max_potential, size_t *ids, void *state)
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

	// Get inverse ray direction and whether each ray component is negative
	const Vec3 inv_d_f = ray.get_inverse_d();
	const std::array<uint32_t, 3> d_is_neg = ray.get_d_is_neg();

	// Load ray origin, inverse direction, and max_t into simd layouts for intersection testing
	const SIMD::float4 ray_o[3] = {ray.o[0], ray.o[1], ray.o[2]};
	const SIMD::float4 inv_d[3] = {inv_d_f[0], inv_d_f[1], inv_d_f[2]};
	const SIMD::float4 max_t {
		ray.max_t
	};

	// Traverse the BVH
	uint32_t hits_so_far = 0;

	while (hits_so_far < max_potential) {
		const Node& n = nodes[node];

		if (!n.child_index) {
			// Leaf node
			ids[hits_so_far++] = node;
		} else {
			// Inner node
			// Test ray against children's bboxes
			bool hit0, hit1;
			SIMD::float4 near_hits;
			uint32_t ti;
			float alpha;
			// Get the time-interpolated bounding box
			const BBox2 b = calc_time_interp(n.time_samples, ray.time, &ti, &alpha) ? lerp(alpha, nodes[node+ti].bounds, nodes[node+ti+1].bounds) : n.bounds;
			// Ray test
			std::tie(hit0, hit1) = b.intersect_ray(ray_o, inv_d, max_t, d_is_neg, &near_hits);
#ifdef GLOBAL_STATS_TOP_LEVEL_BVH_NODE_TESTS
			Global::Stats::bbox_tests += 2;
#endif

			if (hit0 || hit1) {
				bit_stack <<= 1;
				if (hit0 && hit1) {
					if (near_hits[0] < near_hits[1])
						node = child1(node);
					else
						node = child2(node);
					bit_stack |= 1;
				} else if (hit0) {
					node = child1(node);
				} else {
					node = child2(node);
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
