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

BVH::~BVH()
{
	for (size_t i=0; i < next_node; i++) {
		if (nodes[i].flags & IS_LEAF)
			delete nodes[i].data;
	}
}

void BVH::add_primitives(std::vector<Primitive *> &primitives)
{
	size_t start = bag.size();
	size_t added = primitives.size();
	bag.resize(start + added);

	for (size_t i=0; i < added; i++) {
		bag[start + i].init(primitives[i]);
	}
}

bool BVH::finalize()
{
	if (bag.size() == 0)
		return true;
	next_node = 1;
	recursive_build(0, 0, 0, bag.size()-1);
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
uint32_t BVH::split_primitives(size_t first_prim, size_t last_prim, int32_t *axis)
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

	if (axis)
		*axis = max_axis;

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
uint32_t BVH::split_primitives(size_t first_prim, size_t last_prim, int32_t *axis)
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

		if (axis)
			*axis = split_axis;

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
void BVH::recursive_build(size_t parent, size_t me, size_t first_prim, size_t last_prim)
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
		for (size_t i = 0; i < nodes[me].ts; i++) {
			// Copy bounding boxes
			bboxes[next_bbox+i] = bag[first_prim].data->bounds()[i];
		}
		nodes[me].bbox_index = next_bbox;
		next_bbox += nodes[me].ts;

		return;
	}

	// Not a leaf
	uint32_t child1i = next_node;
	uint32_t child2i = next_node + 1;
	next_node += 2;
	nodes[me].child_index = child1i;

	// Create child nodes
	int32_t axis;
	uint32_t split_index = split_primitives(first_prim, last_prim, &axis);
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
	if (nodes[child1i].ts == nodes[child2i].ts) {
		nodes[me].ts = nodes[child1i].ts;
		if ((next_bbox + nodes[me].ts) >= bboxes.size()) // Make sure we have enough space
			bboxes.resize(next_bbox + nodes[me].ts + 256); // Allocate space if not

		for (size_t i = 0; i < nodes[me].ts; i++) {
			// Copy merged bounding boxes
			bboxes[next_bbox+i] =          bboxes[nodes[child1i].bbox_index+i];
			bboxes[next_bbox+i].merge_with(bboxes[nodes[child2i].bbox_index+i]);
		}

	}
	// If children have different number of time samples
	else {
		nodes[me].ts = 1;
		if ((next_bbox + nodes[me].ts) >= bboxes.size()) // Make sure we have enough space
			bboxes.resize(next_bbox + nodes[me].ts + 256); // Allocate space if not

		// Merge children's bboxes to get our bbox
		bboxes[next_bbox] = bboxes[nodes[child1i].bbox_index];
		for (size_t i = 1; i < nodes[child1i].ts; i++) {
			bboxes[next_bbox].merge_with(bboxes[nodes[child1i].bbox_index+i]);
		}
		for (size_t i = 0; i < nodes[child2i].ts; i++) {
			bboxes[next_bbox].merge_with(bboxes[nodes[child2i].bbox_index+i]);
		}
	}
	nodes[me].bbox_index = next_bbox;
	next_bbox += nodes[me].ts;
}


BBoxT &BVH::bounds()
{
	return bbox;
}

bool BVH::intersect_ray(const Ray &ray, Intersection *intersection)
{
	bool hit = false;

	// Traverse the BVH and check for intersections. Yay!
	float hitt0, hitt1;
	uint32_t todo_offset = 0, node = 0;
	uint32_t todo[64];

	while (true) {
		if (intersect_node(node, ray, &hitt0, &hitt1)) {
			if (nodes[node].flags & IS_LEAF) {
				// Trace!
				hit |= nodes[node].data->intersect_ray(ray, intersection);

				// Early out for shadow rays
				if (hit && ray.is_shadow_ray)
					break;

				if (todo_offset == 0)
					break;

				node = todo[--todo_offset];
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

#if 0
uint BVH::get_potential_intersections(const Ray &ray, float tmax, uint max_potential, size_t *ids, void *state)
{
	uint64_t hits = 0;
	uint hits_so_far = 0;
	uint prior_hits = 0;
	if (state) {
		if (static_cast<uint64_t*>(state)[1])
			return 0;
		prior_hits = static_cast<uint64_t*>(state)[0];
	}

	// Traverse the BVH and check for intersections. Yay!
	float hitt0, hitt1;
	uint32_t todo_offset = 0, node = 0;
	uint32_t todo[1000];

	while (true) {
		if (intersect_node(node, ray, &hitt0, &hitt1)) {
			if (nodes[node].flags & IS_LEAF) {
				if (hits_so_far >= prior_hits) {
					ids[hits_so_far-prior_hits] = node;
					hits++;
				}

				hits_so_far++;


				if (todo_offset == 0 || hits_so_far >= max_potential + prior_hits)
					break;

				node = todo[--todo_offset];
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

	if (hits_so_far - prior_hits == 0)
		static_cast<uint64_t*>(state)[1] = 1;
	static_cast<uint64_t*>(state)[0] = hits_so_far;
	return hits_so_far - prior_hits;
}

#else
#define FROM_PARENT 0
#define FROM_SIBLING 1
#define FROM_CHILD 2

// TODO: currently the "ids" returned are node id's, but they should be
// primitive ids.  Similarly, get_primitve() should be changed to take
// primitive ids.
uint BVH::get_potential_intersections(const Ray &ray, float tmax, uint max_potential, size_t *ids, void *state)
{
	uint64_t node;
	uint8_t node_state;

	// Check if it's an empty BVH
	if (nodes.size() == 0)
		return 0;

	// Initialize state
	if (state == nullptr) {
		node = 0;
		node_state = FROM_PARENT;
	} else {
		node = ((uint64_t *)state)[0];
		node_state = ((uint64_t *)state)[1];
	}

	// Traverse the BVH
	bool hit;
	uint32_t hits_so_far = 0;
	float hitt0, hitt1;
	bool finished = false;
	while (hits_so_far < max_potential && !finished) {
		const BVHNode &current = nodes[node];
		const BVHNode &parent = nodes[current.parent_index];

		switch (node_state) {
			case FROM_PARENT:
				hit = intersect_node(node, ray, &hitt0, &hitt1) && hitt0 < tmax;

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

					// If root node (i.e. only one node in the tree),
					// then finished
					if (node == 0) {
						finished = true;
						node_state = FROM_CHILD;
					} else {
						// Go to sibling node
						if (ray.d_is_neg[parent.flags & SPLIT_MASK])
							node--;
						else
							node++;
					}

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
				hit = intersect_node(node, ray, &hitt0, &hitt1) && hitt0 < tmax;

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
	if (state != nullptr) {
		((uint64_t *)state)[0] = node;
		((uint64_t *)state)[1] = node_state;
	}

	// Return the number of primitives accumulated
	return hits_so_far;
}

#endif

