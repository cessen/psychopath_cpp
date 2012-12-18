#include "numtype.h"

#include <iostream>
#include <algorithm>
#include "ray.hpp"
#include "bvh.hpp"
#include <cmath>


#define X_SPLIT 0
#define Y_SPLIT 1
#define Z_SPLIT 2
#define SPLIT_MASK 2
#define IS_LEAF 4

#define SAH

BVH::~BVH()
{
	for (uint_i i=0; i < next_node; i++) {
		if (nodes[i].flags & IS_LEAF)
			delete nodes[i].data;
	}
}

void BVH::add_primitives(std::vector<Primitive *> &primitives)
{
	uint_i start = bag.size();
	uint_i added = primitives.size();
	bag.resize(start + added);

	for (uint_i i=0; i < added; i++) {
		bag[start + i].init(primitives[i]);
	}
}

bool BVH::finalize()
{
	next_node = 1;
	recursive_build(0, 0, 0, bag.size()-1);
	bag.resize(0);
	return true;
}

// TODO: should be changed to fetch based on primitive id, not node id.
Primitive &BVH::get_primitive(uint_i id)
{
	return *(nodes[id].data);
}


struct CompareToMid {
	int32 dim;
	float32 mid;

	CompareToMid(int32 d, float32 m) {
		dim = d;
		mid = m;
	}

	bool operator()(BVHPrimitive &a) const {
		return a.c[dim] < mid;
	}
};

struct CompareDim {
	int32 dim;

	CompareDim(int32 d) {
		dim = d;
	}

	bool operator()(BVHPrimitive &a, BVHPrimitive &b) const {
		return a.c[dim] < b.c[dim];
	}
};



/*
 * Determines the split of the primitives in bag starting
 * at first and ending at last.  May reorder that section of the
 * list.  Used in recursive_build for BVH construction.
 * Returns the split index (last index of the first group).
 */
#ifndef SAH
uint32 BVH::split_primitives(uint_i first_prim, uint_i last_prim, int32 *axis)
{
	// Find the minimum and maximum centroid values on each axis
	Vec3 min, max;
	min = bag[first_prim].c;
	max = bag[first_prim].c;
	for (uint32 i = first_prim+1; i <= last_prim; i++) {
		for (int32 d = 0; d < 3; d++) {
			min[d] = min[d] < bag[i].c[d] ? min[d] : bag[i].c[d];
			max[d] = max[d] > bag[i].c[d] ? max[d] : bag[i].c[d];
		}
	}

	// Find the axis with the maximum extent
	int32 max_axis = 0;
	if ((max.y - min.y) > (max.x - min.x))
		max_axis = 1;
	if ((max.z - min.z) > (max.y - min.y))
		max_axis = 2;

	if (axis)
		*axis = max_axis;

	// Sort and split the list
	float32 pmid = .5f * (min[max_axis] + max[max_axis]);
	BVHPrimitive *mid_ptr = std::partition(&bag[first_prim],
	                                       (&bag[last_prim])+1,
	                                       CompareToMid(max_axis, pmid));

	uint32 split = (mid_ptr - &bag.front());
	if (split > 0)
		split--;

	if (split < first_prim)
		split = first_prim;

	return split;
}

#else

/* SAH-based split */
uint32 BVH::split_primitives(uint_i first_prim, uint_i last_prim, int32 *axis)
{
	uint32 split;

	// Find the minimum and maximum centroid values on each axis
	Vec3 min, max;
	min = bag[first_prim].c;
	max = bag[first_prim].c;
	for (uint32 i = first_prim+1; i <= last_prim; i++) {
		for (int32 d = 0; d < 3; d++) {
			min[d] = min[d] < bag[i].c[d] ? min[d] : bag[i].c[d];
			max[d] = max[d] > bag[i].c[d] ? max[d] : bag[i].c[d];
		}
	}

	const int32 nBuckets = 12;
	if ((last_prim - first_prim) <= 4) {
		// No need to do SAH-based split

		// Find the axis with the maximum extent
		int32 max_axis = 0;
		if ((max.y - min.y) > (max.x - min.x))
			max_axis = 1;
		if ((max.z - min.z) > (max.y - min.y))
			max_axis = 2;

		if (axis)
			*axis = max_axis;

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
		for (uint32 i = first_prim; i <= last_prim; i++) {
			int32 b_x = nBuckets * ((bag[i].c[0] - min[0]) / (max[0] - min[0]));
			int32 b_y = nBuckets * ((bag[i].c[1] - min[1]) / (max[1] - min[1]));
			int32 b_z = nBuckets * ((bag[i].c[2] - min[2]) / (max[2] - min[2]));

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
			for (int32 j=0; j < 3; j++) {
				buckets_x[b_x].bb[0].min[j] = bag[i].bmin[j] < buckets_x[b_x].bb[0].min[j] ? bag[i].bmin[j] : buckets_x[b_x].bb[0].min[j];
				buckets_x[b_x].bb[0].max[j] = bag[i].bmax[j] > buckets_x[b_x].bb[0].max[j] ? bag[i].bmax[j] : buckets_x[b_x].bb[0].max[j];

				buckets_y[b_y].bb[0].min[j] = bag[i].bmin[j] < buckets_y[b_y].bb[0].min[j] ? bag[i].bmin[j] : buckets_y[b_y].bb[0].min[j];
				buckets_y[b_y].bb[0].max[j] = bag[i].bmax[j] > buckets_y[b_y].bb[0].max[j] ? bag[i].bmax[j] : buckets_y[b_y].bb[0].max[j];

				buckets_z[b_z].bb[0].min[j] = bag[i].bmin[j] < buckets_z[b_z].bb[0].min[j] ? bag[i].bmin[j] : buckets_z[b_z].bb[0].min[j];
				buckets_z[b_z].bb[0].max[j] = bag[i].bmax[j] > buckets_z[b_z].bb[0].max[j] ? bag[i].bmax[j] : buckets_z[b_z].bb[0].max[j];
			}
		}

		// Calculate the cost of each split
		float32 cost_x[nBuckets-1];
		float32 cost_y[nBuckets-1];
		float32 cost_z[nBuckets-1];
		for (int32 i = 0; i < nBuckets-1; ++i) {
			BBoxT b0_x, b1_x;
			BBoxT b0_y, b1_y;
			BBoxT b0_z, b1_z;
			int32 count0_x = 0, count1_x = 0;
			int32 count0_y = 0, count1_y = 0;
			int32 count0_z = 0, count1_z = 0;

			b0_x.copy(buckets_x[0].bb);
			b0_y.copy(buckets_y[0].bb);
			b0_z.copy(buckets_z[0].bb);
			for (int32 j = 0; j <= i; ++j) {
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
			for (int32 j = i+1; j < nBuckets; ++j) {
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
		float32 minCost = cost_x[0];
		int32 split_axis = 0;
		uint32 minCostSplit = 0;
		// X
		for (int32 i = 1; i < nBuckets-1; ++i) {
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

		if (axis)
			*axis = split_axis;

		float32 pmid = min[split_axis] + (((max[split_axis] - min[split_axis]) / nBuckets) * (minCostSplit+1));
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
void BVH::recursive_build(uint_i parent, uint_i me, uint_i first_prim, uint_i last_prim)
{
	// Need to allocate more node space?
	if (me >= nodes.size())
		nodes.resize(nodes.size()+256);

	nodes[me].flags = 0;
	nodes[me].parent_index = parent;

	// Leaf node?
	if (first_prim == last_prim) {
		nodes[me].flags |= IS_LEAF;
		nodes[me].data = bag[first_prim].data;

		// Get bounding boxes
		nodes[me].ts = bag[first_prim].data->bounds().size();
		if ((next_bbox + nodes[me].ts) >= bboxes.size()) // Make sure we have enough space
			bboxes.resize(next_bbox + nodes[me].ts + 256); // Allocate space if not
		for (uint_i i = 0; i < nodes[me].ts; i++) {
			// Copy bounding boxes
			bboxes[next_bbox+i] = bag[first_prim].data->bounds()[i];
		}
		nodes[me].bbox_index = next_bbox;
		next_bbox += nodes[me].ts;

		return;
	}

	// Not a leaf
	uint32 child1i = next_node;
	uint32 child2i = next_node + 1;
	next_node += 2;
	nodes[me].child_index = child1i;

	// Create child nodes
	int32 axis;
	uint32 split_index = split_primitives(first_prim, last_prim, &axis);
	switch (axis) {
		case 0:
			nodes[me].flags |= X_SPLIT;
			break;

		case 1:
			nodes[me].flags |= Y_SPLIT;
			break;

		case 2:
			nodes[me].flags |= Z_SPLIT;
			break;

		default:
			nodes[me].flags |= X_SPLIT;
			break;
	}

	recursive_build(me, child1i, first_prim, split_index);
	recursive_build(me, child2i, split_index+1, last_prim);

	// Calculate bounds
	// If both children have same number of time samples
	/*if (nodes[child1i].ts == nodes[child2i].ts) {
		nodes[me].ts = nodes[child1i].ts;
		if ((next_bbox + nodes[me].ts) >= bboxes.size()) // Make sure we have enough space
			bboxes.resize(next_bbox + nodes[me].ts + 256); // Allocate space if not

		for (uint_i i = 0; i < nodes[me].ts; i++) {
			// Copy merged bounding boxes
			bboxes[next_bbox+i] =          bboxes[nodes[child1i].bbox_index+i];
			bboxes[next_bbox+i].merge_with(bboxes[nodes[child2i].bbox_index+i]);
		}

	}
	// If children have different number of time samples
	else {*/
	nodes[me].ts = 1;
	if ((next_bbox + nodes[me].ts) >= bboxes.size()) // Make sure we have enough space
		bboxes.resize(next_bbox + nodes[me].ts + 256); // Allocate space if not

	// Merge children's bboxes to get our bbox
	bboxes[next_bbox] = bboxes[nodes[child1i].bbox_index];
	for (uint_i i = 1; i < nodes[child1i].ts; i++) {
		bboxes[next_bbox].merge_with(bboxes[nodes[child1i].bbox_index+i]);
	}
	for (uint_i i = 0; i < nodes[child2i].ts; i++) {
		bboxes[next_bbox].merge_with(bboxes[nodes[child2i].bbox_index+i]);
	}
	//}
	nodes[me].bbox_index = next_bbox;
	next_bbox += nodes[me].ts;
}


BBoxT &BVH::bounds()
{
	return bbox;
}

bool BVH::intersect_ray(Ray &ray, Intersection *intersection)
{
	bool hit = false;
	std::vector<Primitive *> temp_prim;

	// Traverse the BVH and check for intersections. Yay!
	float32 hitt0, hitt1;
	uint32 todo_offset = 0, node = 0;
	uint32 todo[64];

	while (true) {
		if (intersect_node(node, ray, &hitt0, &hitt1)) {
			if (nodes[node].flags & IS_LEAF) {
				if (nodes[node].data->is_traceable(ray.min_width(hitt0, hitt1))) {
					// Trace!
					hit |= nodes[node].data->intersect_ray(ray, intersection);

					// Early out for shadow rays
					if (hit && ray.is_shadow_ray)
						break;

					if (todo_offset == 0)
						break;

					node = todo[--todo_offset];
				} else {
					// Split!
					nodes[node].data->refine(temp_prim);
					add_primitives(temp_prim);
					temp_prim.resize(0);
					delete nodes[node].data;

					recursive_build(nodes[node].parent_index, node, 0, bag.size()-1);
					bag.resize(0);
				}
			} else {
				// Put far BVH node on todo stack, advance to near node
				if (ray.d_is_neg[nodes[node].flags & SPLIT_MASK]) {
					todo[todo_offset++] = nodes[node].child_index;
					node = nodes[node].child_index + 1;
				} else {
					todo[todo_offset++] = nodes[node].child_index + 1;
					node = nodes[node].child_index;
				}
			}
		} else {
			if (todo_offset == 0)
				break;

			node = todo[--todo_offset];
		}
	}

	return hit;
}

#define FROM_PARENT 0
#define FROM_SIBLING 1
#define FROM_CHILD 2

// TODO: currently the "ids" returned are node id's, but they should be
// primitive ids.  Similarly, get_primitve() should be changed to take
// primitive ids.
uint BVH::get_potential_intersections(const Ray &ray, uint max_potential, uint_i *ids, void *state)
{
	uint64 node;
	uint8 node_state;

	// Initialize state
	if (state == NULL) {
		node = 0;
		node_state = FROM_PARENT;
	} else {
		node = ((uint64 *)state)[0];
		node_state = ((uint64 *)state)[1];
	}

	// Traverse the BVH
	bool hit;
	uint32 hits_so_far = 0;
	float32 hitt0, hitt1;
	bool finished = false;
	while (hits_so_far < max_potential && !finished) {
		const BVHNode &current = nodes[node];
		const BVHNode &parent = nodes[current.parent_index];

		switch (node_state) {
			case FROM_PARENT:
				hit = intersect_node(node, ray, &hitt0, &hitt1);

				if (!hit) {
					// If ray misses BBox
					if (node == 0) {
						// If node is root node, finished
						finished = true;
						node_state = FROM_CHILD;
					} else {
						// Go to sibling node
						if (ray.d_is_neg[parent.flags & SPLIT_MASK])
							node--;
						else
							node++;

						// State: from_sibling
						node_state = FROM_SIBLING;
					}
				} else if (current.flags & IS_LEAF) {
					// If ray hits BBox and node is leaf
					ids[hits_so_far] = node;
					hits_so_far++;

					// Go to sibling node
					if (ray.d_is_neg[parent.flags & SPLIT_MASK])
						node--;
					else
						node++;

					// State: from_sibling
					node_state = FROM_SIBLING;
				} else {
					// If ray hits BBox and node is not leaf
					// Go to near child
					if (ray.d_is_neg[current.flags & SPLIT_MASK])
						node = current.child_index+1;
					else
						node = current.child_index;

					// State: from_parent
					node_state = FROM_PARENT;
				}

				break;

			case FROM_SIBLING:
				hit = intersect_node(node, ray, &hitt0, &hitt1);

				if (!hit) {
					// If ray misses BBox, go to parent node.
					node = current.parent_index;

					// State: from_child
					node_state = FROM_CHILD;
				} else if (current.flags & IS_LEAF) {
					// If ray hits BBox and node is leaf
					ids[hits_so_far] = node;
					hits_so_far++;

					// Go to parent node
					node = current.parent_index;

					// State: from_child
					node_state = FROM_CHILD;
				} else {
					// If ray hits BBox and node is not leaf
					// Go to near child
					if (ray.d_is_neg[current.flags & SPLIT_MASK])
						node = current.child_index+1;
					else
						node = current.child_index;

					// State: from_parent
					node_state = FROM_PARENT;
				}

				break;

			case FROM_CHILD:
				if (node == 0) {
					// If root node, finished
					finished = true;
				} else if (ray.d_is_neg[parent.flags & SPLIT_MASK] && node == (parent.child_index+1)) {
					// If node is the near child of its parent
					// Go to sibling
					node--;

					// State: from_sibling
					node_state = FROM_SIBLING;
				} else if (!(ray.d_is_neg[parent.flags & SPLIT_MASK]) && node == (parent.child_index)) {
					// If node is the near child of its parent
					// Go to sibling
					node++;

					// State: from_sibling
					node_state = FROM_SIBLING;
				} else {
					// If node is the far child of its parent
					// Go to parent
					node = current.parent_index;

					// State: from_child
					node_state = FROM_CHILD;
				}

				break;
		}
	}

	// Store state
	if (state != NULL) {
		((uint64 *)state)[0] = node;
		((uint64 *)state)[1] = node_state;
	}

	// Return the number of primitives accumulated
	return hits_so_far;
}

