#include <iostream>
#include <algorithm>
#include <memory>
#include <tuple>
#include <iterator>
#include <cmath>

#include "numtype.h"
#include "bvh4.hpp"

#include "simd.hpp"
#include "ray.hpp"
#include "assembly.hpp"
#include "utils.hpp"


void BVH4::build(const Assembly& assembly)
{
	// Build a normal BVH as a starting point
	BVH bvh;
	bvh.build(assembly);

	if (bvh.nodes.size() == 0)
		return;


	// Pack BVH into BVH4
	nodes.push_back(Node());

	for (size_t bni = 0; bni < bvh.nodes.size(); ++bni) {
		BVH::Node& bn = bvh.nodes[bni];
		size_t ni = nodes.size() - 1; // Node index

		// Skip nodes that are intermediaries
		if (bn.flags & IS_SKIP)
			continue;

		// Set the values that don't depend on whether this
		// is a leaf node or not.
		if (bn.flags & IS_2ND) {
			nodes[bn.parent_index].child_indices[0] = ni;
		} else if (bn.flags & IS_3RD) {
			nodes[bn.parent_index].child_indices[1] = ni;
		} else if (bn.flags & IS_4TH) {
			nodes[bn.parent_index].child_indices[2] = ni;
		}

		// Set the values that _do_ depend on whether this is
		// a leaf node or not.
		if (bn.flags & BVH::IS_LEAF) {
			nodes[ni].child_indices[0] = 0; // Indicates that this is a leaf node
			nodes[ni].data_index = bn.data_index;
			nodes.push_back(Node());
		} else {
			// Collect children
			int child_count = 0;
			BVH::Node* children[4] = {nullptr, nullptr, nullptr, nullptr};
			const int ci1 = bni+1;
			const int ci2 = bn.child_index;
			if (bvh.nodes[ci1].flags & BVH::IS_LEAF) {
				children[child_count++] = &(bvh.nodes[ci1]);
			} else {
				bvh.nodes[ci1].flags |= IS_SKIP;
				children[child_count++] = &(bvh.nodes[ci1+1]);
				children[child_count++] = &(bvh.nodes[bvh.nodes[ci1].child_index]);
			}
			if (bvh.nodes[ci2].flags & BVH::IS_LEAF) {
				children[child_count++] = &(bvh.nodes[ci2]);
			} else {
				bvh.nodes[ci2].flags |= IS_SKIP;
				children[child_count++] = &(bvh.nodes[ci2+1]);
				children[child_count++] = &(bvh.nodes[bvh.nodes[ci2].child_index]);
			}

			// Let the children know their place in the world
			if (children[1])
				children[1]->flags |= IS_2ND;
			if (children[2])
				children[2]->flags |= IS_3RD;
			if (children[3])
				children[3]->flags |= IS_4TH;

			// Set the parent index fields in the child build nodes
			// to refer to the parent Node instead of the parent BVH::Node
			for (int i = 0; i < child_count; ++i)
				children[i]->parent_index = ni;

			// Figure out if the children have the same number of time samples
			bool equal_time_samples = true;
			int most_time_samples = children[0]->ts;
			for (int i = 1; i < child_count; ++i) {
				equal_time_samples = equal_time_samples && (children[i-1]->ts == children[i]->ts);
				if (children[i]->ts > most_time_samples)
					most_time_samples = children[i]->ts;
			}

			// If children have same number of time samples, easy
			if (equal_time_samples) {
				nodes[ni].ts = children[0]->ts;
				for (uint16_t i = 0; i < children[0]->ts; ++i) {
					switch (child_count) {
						case 2:
							nodes.back().bounds = BBox4(bvh.bboxes[children[0]->bbox_index+i], bvh.bboxes[children[1]->bbox_index+i], BBox(), BBox());
							break;
						case 3:
							nodes.back().bounds = BBox4(bvh.bboxes[children[0]->bbox_index+i], bvh.bboxes[children[1]->bbox_index+i], bvh.bboxes[children[2]->bbox_index+i], BBox());
							break;
						case 4:
							nodes.back().bounds = BBox4(bvh.bboxes[children[0]->bbox_index+i], bvh.bboxes[children[1]->bbox_index+i], bvh.bboxes[children[2]->bbox_index+i], bvh.bboxes[children[3]->bbox_index+i]);
							break;
					}
					nodes.push_back(Node());
				}
			}
			// If children have different number of time samples,
			// interpolate to the same number
			else {
				nodes[ni].ts = most_time_samples;
				const float s = nodes[ni].ts - 1;

				for (uint16_t i = 0; i < nodes[ni].ts; ++i) {
					BBox bb[4];

					for (int ci = 0; ci < child_count; ++ci) {
						if (children[ci]->ts == most_time_samples) {
							bb[ci] = bvh.bboxes[children[ci]->bbox_index+i];
						} else {
							auto cbegin = bvh.bboxes.cbegin() + children[ci]->bbox_index;
							auto cend = cbegin + children[ci]->ts;
							bb[ci] = lerp_seq(i/s, cbegin, cend);
						}
					}

					switch (child_count) {
						case 2:
							nodes.back().bounds = BBox4(bb[0], bb[1], BBox(), BBox());
							break;
						case 3:
							nodes.back().bounds = BBox4(bb[0], bb[1], bb[2], BBox());
							break;
						case 4:
							nodes.back().bounds = BBox4(bb[0], bb[1], bb[2], bb[3]);
							break;
					}

					nodes.push_back(Node());
				}
			}
		}
	}
	nodes.pop_back();
	nodes.shrink_to_fit();


	// Store top-level bounds
	auto begin = bvh.bboxes.begin() + bvh.nodes[0].bbox_index;
	auto end = begin + bvh.nodes[0].ts;
	_bounds.clear();
	_bounds.insert(_bounds.begin(), begin, end);
}



std::tuple<Ray*, Ray*, size_t> BVH4StreamTraverser::next_object()
{
	while (stack_ptr >= 0) {
		if (bvh->is_leaf(node_stack[stack_ptr])) {
			ray_stack[stack_ptr].second = mutable_partition(ray_stack[stack_ptr].first, ray_stack[stack_ptr].second, [&](Ray& ray) {
				return !ray.is_done() && (first_call || ray.trav_stack.pop());
			});

			if (std::distance(ray_stack[stack_ptr].first, ray_stack[stack_ptr].second) > 0) {
				auto rv = std::make_tuple(&(*ray_stack[stack_ptr].first), &(*ray_stack[stack_ptr].second), bvh->nodes[node_stack[stack_ptr]].data_index);
				--stack_ptr;
				return rv;
			} else {
				--stack_ptr;
			}
		} else {
			const int num_children = bvh->child_count(node_stack[stack_ptr]);
			const auto node_begin = bvh->nodes.cbegin() + node_stack[stack_ptr];
			const auto node_end = node_begin + bvh->nodes[node_stack[stack_ptr]].ts;

			SIMD::float4 near_hits; // For storing near-hit data in the ray-test loop below
			bool rot_set = false;
			int rot = 0;

			// Test rays against current node's children
			ray_stack[stack_ptr].second = mutable_partition(ray_stack[stack_ptr].first, ray_stack[stack_ptr].second, [&](Ray& ray) {
				if (!ray.is_done() && (first_call || ray.trav_stack.pop())) {
					// Get the time-interpolated bounding box
					const BBox4 b = lerp_seq(ray.time, node_begin, node_end).bounds;

					// Ray test
					const auto hit_mask = b.intersect_ray(ray, &near_hits);

					// Push results to the bit stack
					if (hit_mask != 0) {
						if (!rot_set) {
							rot_set = true;
							for (int i = 1; i < num_children; ++i) {
								if (near_hits[i] < near_hits[rot])
									rot = i;
							}
						}
						ray.trav_stack.push((hit_mask >> rot) | (hit_mask << (num_children-rot)), num_children);
					}

					// Return whether the ray hit any of the child nodes
					return hit_mask != 0;
				} else {
					return false;
				}
			});

			if (first_call)
				first_call = false;

			// If any rays hit, traverse deeper
			if (std::distance(ray_stack[stack_ptr].first, ray_stack[stack_ptr].second) > 0) {
				const auto node_i = node_stack[stack_ptr];

				for (int i = 0; i < num_children; ++i) {
					ray_stack[stack_ptr+i] = ray_stack[stack_ptr];
					node_stack[stack_ptr+i] = bvh->child(node_i, num_children-1-((i+num_children-rot)%num_children));
				}

				stack_ptr += num_children - 1;
			}
			// If no rays hit, go to next stack item
			else {
				--stack_ptr;
			}
		}
	}

	// Finished traversal
	return std::make_tuple(rays_end, rays_end, 0);
}
