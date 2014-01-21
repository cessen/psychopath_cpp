#ifndef BVH_HPP
#define BVH_HPP

#include "numtype.h"
#include "global.hpp"

#include <stdlib.h>
#include <iostream>
#include <vector>
#include <memory>
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
class BVH: public Collection
{
public:
	virtual ~BVH() {};

	virtual void add_primitives(std::vector<std::unique_ptr<Primitive>>* primitives);
	virtual bool finalize();
	virtual size_t max_primitive_id() const;
	virtual Primitive &get_primitive(size_t id);
	virtual uint get_potential_intersections(const Ray &ray, float tmax, uint max_potential, size_t *ids, void *state);
	virtual size_t ray_state_size() {
		return 16;
	}

	/*
	 * A node of a bounding volume hierarchy.
	 * Contains a bounding box, a flag for whether
	 * it's a leaf or not, a pointer to its first
	 * child, and it's data if it's a leaf.
	 */
	struct Node {
		size_t bbox_index = 0;
		union {
			size_t child_index;
			Primitive *data = nullptr;
		};
		size_t parent_index = 0;
		uint16_t ts = 0; // Time sample count
		uint16_t flags = 0;
	};

	struct BucketInfo {
		BucketInfo() {
			count = 0;
		}
		size_t count;
		BBoxT bb;
	};

	/*
	 * Used to store primitives that have yet to be
	 * inserted into the hierarchy.
	 * Contains the time 0.5 bounds of the primitive and it's centroid.
	 */
	class BVHPrimitive
	{
	public:
		Primitive *data;
		Vec3 bmin, bmax, c;

		BVHPrimitive() {
			data = nullptr;
		}

		BVHPrimitive(Primitive *prim) {
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

private:
	BBoxT bbox;
	std::vector<Node> nodes;
	std::vector<BBox> bboxes;
	std::vector<BVHPrimitive> bag;  // Temporary holding spot for primitives not yet added to the hierarchy

	/**
	 * @brief Tests whether a ray intersects a node or not.
	 */
	inline bool intersect_node(const uint64_t node_i, const Ray& ray, const Vec3& d_inv, const std::array<uint32_t, 3>& d_sign, float *near_t, float *far_t) const {
#ifdef GLOBAL_STATS_TOP_LEVEL_BVH_NODE_TESTS
		Global::Stats::top_level_bvh_node_tests++;
#endif
		const Node& node = nodes[node_i];
		const BBox b = lerp_seq<BBox, decltype(bboxes)::const_iterator >(ray.time, bboxes.cbegin() + node.bbox_index, node.ts);
		return b.intersect_ray(ray, d_inv, d_sign, near_t, far_t);
	}

	/**
	 * @brief Returns the index of the first child
	 * of the node with the given index.
	 */
	inline size_t child1(const size_t node_i) const {
		return node_i + 1;
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
		const size_t parent_i = nodes[node_i].parent_index;
		if (node_i == (parent_i + 1))
			return nodes[parent_i].child_index;
		else
			return parent_i + 1;
	}

	size_t split_primitives(size_t first_prim, size_t last_prim);
	size_t recursive_build(size_t parent, size_t first_prim, size_t last_prim);
};




#endif
