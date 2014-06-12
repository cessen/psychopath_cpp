#include "numtype.h"

#include <iostream>
#include <algorithm>
#include <memory>
#include <tuple>
#include <iterator>
#include <cmath>

#include "numtype.h"

#include "simd.hpp"
#include "ray.hpp"
#include "bvh2.hpp"
#include "assembly.hpp"



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
}



std::tuple<Ray*, Ray*, size_t> BVH2StreamTraverser::next_object()
{
	// If there aren't any objects in the scene, return finished
	if (bvh->nodes.size() == 0)
		return std::make_tuple(rays_end, rays_end, 0);

	while (stack_ptr >= 0) {
		if (bvh->is_leaf(node_stack[stack_ptr])) {
			ray_stack[stack_ptr].first = std::partition(ray_stack[stack_ptr].first, ray_stack[stack_ptr].second, [this](Ray& ray) {
				return !ray.trav_stack.pop();
			});

			if (std::distance(ray_stack[stack_ptr].first, ray_stack[stack_ptr].second) > 0) {
				auto rv = std::make_tuple(&(*ray_stack[stack_ptr].first), &(*ray_stack[stack_ptr].second), bvh->nodes[node_stack[stack_ptr]].data_index);
				--stack_ptr;
				return rv;
			}
		} else {
			// Test rays against current node's children
			ray_stack[stack_ptr].first = std::partition(ray_stack[stack_ptr].first, ray_stack[stack_ptr].second, [this](Ray& ray) {
				if (ray.trav_stack.pop() && (ray.flags & Ray::DONE) == 0) {
					// Load ray origin, inverse direction, and max_t into simd layouts for intersection testing
					const SIMD::float4 ray_o[3] = {ray.o[0], ray.o[1], ray.o[2]};
					const SIMD::float4 inv_d[3] = {ray.d_inv[0], ray.d_inv[1], ray.d_inv[2]};
					const SIMD::float4 max_t {
						ray.max_t
					};

					uint64_t hit_mask;
					SIMD::float4 near_hits;

					// Get the time-interpolated bounding box
					const auto cbegin = bvh->nodes.cbegin() + node_stack[stack_ptr];
					const auto cend = cbegin + bvh->nodes[node_stack[stack_ptr]].ts;
					const BBox2 b = lerp_seq(ray.time, cbegin, cend).bounds;

					// Ray test
					hit_mask = b.intersect_ray(ray_o, inv_d, max_t, ray.d_sign, &near_hits);

					if (hit_mask != 0)
						ray.trav_stack.push(hit_mask, 2);

					return hit_mask == 0;
				} else {
					return true;
				}
			});

			// If any rays hit, traverse deeper
			if (std::distance(ray_stack[stack_ptr].first, ray_stack[stack_ptr].second) > 0) {
				ray_stack[stack_ptr+1] = ray_stack[stack_ptr];

				node_stack[stack_ptr+1] = bvh->child1(node_stack[stack_ptr]);
				node_stack[stack_ptr] = bvh->child2(node_stack[stack_ptr]);

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