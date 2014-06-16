#include <iostream>
#include <algorithm>
#include <memory>
#include <tuple>
#include <iterator>
#include <cmath>

#include "numtype.h"
#include "bvh2.hpp"

#include "simd.hpp"
#include "ray.hpp"
#include "assembly.hpp"
#include "utils.hpp"



void BVH2::build(const Assembly& assembly)
{
	// Build a normal BVH as a starting point
	BVH bvh;
	bvh.build(assembly);

	if (bvh.nodes.size() == 0)
		return;

	// Pack BVH into more efficient BVH2
	nodes.push_back(Node());
	for (size_t bni = 0; bni < bvh.nodes.size(); ++bni) {
		BVH::Node& bn = bvh.nodes[bni];
		size_t ni = nodes.size() - 1; // Node index

		// Set the values that don't depend on whether this
		// is a leaf node or not.
		if (bn.flags & IS_RIGHT)
			nodes[bn.parent_index].child_index = ni;  // Set parent's child_index field to point to this

		// Set the values that _do_ depend on whether this is
		// a leaf node or not.
		if (bn.flags & BVH::IS_LEAF) {
			nodes[ni].child_index = 0; // Indicates that this is a leaf node
			nodes[ni].data_index = bn.data_index;
			nodes.push_back(Node());
		} else {
			BVH::Node& child1 = bvh.nodes[bvh.child1(bni)];
			BVH::Node& child2 = bvh.nodes[bvh.child2(bni)];

			// Let right child know that it's right
			child2.flags |= IS_RIGHT;

			// Set the parent index fields in the child build nodes
			// to refer to the parent Node instead of the parent BVH::Node
			child1.parent_index = ni;
			child2.parent_index = ni;

			// If children have same number of time samples, easy
			if (child1.ts == child2.ts) {
				nodes[ni].ts = child1.ts;
				for (uint16_t i = 0; i < child1.ts; ++i) {
					nodes.back().bounds = BBox2(bvh.bboxes[child1.bbox_index+i], bvh.bboxes[child2.bbox_index+i]);
					nodes.push_back(Node());
				}
			}
			// If children have different number of time samples,
			// interpolate one or the other
			else if (child1.ts > child2.ts) {
				nodes[ni].ts = child1.ts;
				const float s = child1.ts - 1;
				auto cbegin = bvh.bboxes.cbegin() + child2.bbox_index;
				auto cend = cbegin + child2.ts;

				for (uint16_t i = 0; i < child1.ts; ++i) {
					nodes.back().bounds = BBox2(bvh.bboxes[child1.bbox_index+i], lerp_seq(i/s, cbegin, cend));
					nodes.push_back(Node());
				}
			} else {
				nodes[ni].ts = child2.ts;
				const float s = child2.ts - 1;
				auto cbegin = bvh.bboxes.cbegin() + child1.bbox_index;
				auto cend = cbegin + child1.ts;

				for (uint16_t i = 0; i < child2.ts; ++i) {
					nodes.back().bounds = BBox2(lerp_seq(i/s, cbegin, cend), bvh.bboxes[child2.bbox_index+i]);
					nodes.push_back(Node());
				}
			}
		}
	}

	// Store top-level bounds
	auto begin = bvh.bboxes.begin() + bvh.nodes[0].bbox_index;
	auto end = begin + bvh.nodes[0].ts;
	_bounds.clear();
	_bounds.insert(_bounds.begin(), begin, end);
}



std::tuple<Ray*, Ray*, size_t> BVH2StreamTraverser::next_object()
{
	// If there aren't any objects in the scene, return finished
	if (bvh->nodes.size() == 0)
		return std::make_tuple(rays_end, rays_end, 0);

	while (stack_ptr >= 0) {
		if (bvh->is_leaf(node_stack[stack_ptr])) {
			ray_stack[stack_ptr].second = mutable_partition(ray_stack[stack_ptr].first, ray_stack[stack_ptr].second, [&](Ray& ray) {
				if (!first_call)
					return ray.trav_stack.pop() && (ray.flags & Ray::DONE) == 0;
				else {
					return (ray.flags & Ray::DONE) == 0;
				}
			});

			if (first_call)
				first_call = false;

			if (std::distance(ray_stack[stack_ptr].first, ray_stack[stack_ptr].second) > 0) {
				auto rv = std::make_tuple(&(*ray_stack[stack_ptr].first), &(*ray_stack[stack_ptr].second), bvh->nodes[node_stack[stack_ptr]].data_index);
				--stack_ptr;
				return rv;
			} else {
				--stack_ptr;
			}
		} else {
			const auto cbegin = bvh->nodes.cbegin() + node_stack[stack_ptr];
			const auto cend = cbegin + bvh->nodes[node_stack[stack_ptr]].ts;

			SIMD::float4 near_hits;
			bool flip_set = false;
			bool flip = false;

			// Test rays against current node's children
			ray_stack[stack_ptr].second = mutable_partition(ray_stack[stack_ptr].first, ray_stack[stack_ptr].second, [&](Ray& ray) {
				if ((first_call || ray.trav_stack.pop()) && (ray.flags & Ray::DONE) == 0) {
					// Get the time-interpolated bounding box
					const BBox2 b = lerp_seq(ray.time, cbegin, cend).bounds;

					// Ray test
					const auto hit_mask = b.intersect_ray(ray, &near_hits);

					if (hit_mask != 0) {
						if (!flip_set) {
							flip_set = true;
							flip = near_hits[0] > near_hits[1];
						}

						if (flip)
							ray.trav_stack.push((hit_mask >> 1) | (hit_mask << 1), 2);
						else
							ray.trav_stack.push(hit_mask, 2);
					}

					return hit_mask != 0;
				} else {
					return false;
				}
			});

			if (first_call)
				first_call = false;

			// If any rays hit, traverse deeper
			if (std::distance(ray_stack[stack_ptr].first, ray_stack[stack_ptr].second) > 0) {
				ray_stack[stack_ptr+1] = ray_stack[stack_ptr];

				if (flip) {
					node_stack[stack_ptr+1] = bvh->child2(node_stack[stack_ptr]);
					node_stack[stack_ptr] = bvh->child1(node_stack[stack_ptr]);
				} else {
					node_stack[stack_ptr+1] = bvh->child1(node_stack[stack_ptr]);
					node_stack[stack_ptr] = bvh->child2(node_stack[stack_ptr]);
				}

				++stack_ptr;
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