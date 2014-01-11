#ifndef BVH2_HPP
#define BVH2_HPP

#include "numtype.h"
#include "global.hpp"

#include <stdlib.h>
#include <iostream>
#include <vector>
#include <deque>
#include <memory>
#include <tuple>
#include "primitive.hpp"
#include "collection.hpp"
#include "ray.hpp"
#include "bbox.hpp"
#include "utils.hpp"
#include "vector.hpp"
#include "chunked_array.hpp"




/*
 * A bounding volume hierarchy.
 */
class BVH2: public Collection
{
public:
	virtual ~BVH2() {};

	virtual void add_primitives(std::vector<std::unique_ptr<Primitive>>* primitives);
	virtual bool finalize();
	virtual size_t max_primitive_id() const;
	virtual Primitive &get_primitive(size_t id);
	virtual uint get_potential_intersections(const Ray &ray, float tmax, uint max_potential, size_t *ids, void *state);
	virtual size_t ray_state_size() {
		return 16;
	}

	struct Node {
		size_t child_index = 0; // When zero, indicates that this is a leaf node
		size_t parent_index = 0;
		size_t sibling_index = 0;
		size_t time_samples = 0;
		union {
			// If the node is a leaf, we don't need the bounds.
			// If the node is not a leaf, it doesn't have Primitive data.
			BBox bounds[2] = {BBox(), BBox()};
			Primitive *data;
		};

		Node() {}

		Node(const Node& n):
			child_index {n.child_index},
		            parent_index {n.parent_index},
		            sibling_index {n.sibling_index},
		time_samples {n.time_samples} {
			bounds[0] = n.bounds[0];
			bounds[1] = n.bounds[1];
		}
	};

	/*
	 * A node for building the bounding volume hierarchy.
	 * Contains a bounding box, a flag for whether
	 * it's a leaf or not, a pointer to its first
	 * child, and it's data if it's a leaf.
	 */
	struct BuildNode {
		size_t bbox_index = 0;
		union {
			size_t child_index;
			Primitive *data = nullptr;
		};
		size_t parent_index = 0;
		uint16_t ts = 0; // Time sample count
		uint16_t flags = 0;
	};

	/*
	 * Used to store primitives that have yet to be
	 * inserted into the hierarchy.
	 * Contains the time 0.5 bounds of the primitive and it's centroid.
	 */
	class BuildPrimitive
	{
	public:
		Primitive *data;
		Vec3 bmin, bmax, c;

		BuildPrimitive() {
			data = nullptr;
		}

		BuildPrimitive(Primitive *prim) {
			init(prim);
		}

		void init(Primitive *prim) {
			data = prim;

			// Get bounds at time 0.5
			BBox mid_bb = data->bounds().at_time(0.5);
			bmin = mid_bb.min;
			bmax = mid_bb.max;

			// Get centroid
			c = (bmin * 0.5) + (bmax * 0.5);
		}
	};

	struct BucketInfo {
		BucketInfo() {
			count = 0;
		}
		size_t count;
		BBoxT bb;
	};

private:
	BBoxT bbox;
	std::vector<Node> nodes;
	std::deque<BuildNode> build_nodes;
	std::deque<BBox> build_bboxes;
	std::deque<BuildPrimitive> prim_bag;  // Temporary holding spot for primitives not yet added to the hierarchy

	/**
	 * @brief Tests a ray against a node's children.
	 */
	inline std::tuple<bool, bool> intersect_node(const uint64_t node_i, const Ray& ray, float max_t, const Vec3 inv_d, const std::array<uint32_t, 3> d_is_neg, float *ts) const {
		const Node& node = nodes[node_i];
		uint32_t ti;
		float alpha;

		BBox b1;
		BBox b2;
		if (calc_time_interp(node.time_samples, ray.time, &ti, &alpha)) {
			b1 = lerp(alpha, nodes[node_i+ti].bounds[0], nodes[node_i+ti+1].bounds[0]);
			b2 = lerp(alpha, nodes[node_i+ti].bounds[1], nodes[node_i+ti+1].bounds[1]);
		} else {
			b1 = node.bounds[0];
			b2 = node.bounds[1];
		}

#ifdef GLOBAL_STATS_TOP_LEVEL_BVH_NODE_TESTS
		Global::Stats::bbox_tests += 2;
#endif

		return std::make_tuple(b1.intersect_ray(ray, inv_d, d_is_neg, ts, ts+1, &max_t), b2.intersect_ray(ray, inv_d, d_is_neg, ts+2, ts+3, &max_t));
	}

	/**
	 * @brief Returns the index of the first child
	 * of the node with the given index.
	 */
	inline size_t child1(const size_t node_i) const {
		return node_i + nodes[node_i].time_samples;
	}

	/**
	 * @brief Returns the index of the second child
	 * of the node with the given index.
	 */
	inline size_t child2(const size_t node_i) const {
		return nodes[node_i].child_index;
	}

	/**
	 * @brief Returns the index of the sibling
	 * of the node with the given index.
	 */
	inline size_t sibling(const size_t node_i) const {
		return nodes[node_i].sibling_index;
	}

	size_t split_primitives(size_t first_prim, size_t last_prim);
	size_t recursive_build(size_t parent, size_t first_prim, size_t last_prim);
	void pack();
};




#endif // BVH2_HPP
