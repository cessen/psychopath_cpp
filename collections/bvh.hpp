#ifndef BVH_HPP
#define BVH_HPP

#include "numtype.h"
#include "global.hpp"

#include <stdlib.h>
#include <iostream>
#include <vector>
#include "primitive.hpp"
#include "collection.hpp"
#include "ray.hpp"
#include "bbox.hpp"
#include "utils.hpp"
#include "vector.hpp"
#include "chunked_array.hpp"


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







/*
 * A bounding volume hierarchy.
 */
class BVH: public Collection
{
	/*
	 * A node of a bounding volume hierarchy.
	 * Contains a bounding box, a flag for whether
	 * it's a leaf or not, a pointer to its first
	 * child, and it's data if it's a leaf.
	 */
	struct Node {
		size_t bbox_index;
		union {
			size_t child_index;
			Primitive *data;
		};
		size_t parent_index;
		uint16_t ts; // Time sample count
		uint16_t flags;

		Node() {
			bbox_index = 0;
			child_index = 0;
			data = nullptr;
			ts = 0;
			flags = 0;
		}
	};

private:
	BBoxT bbox;
	ChunkedArray<Node> nodes;
	ChunkedArray<BBox> bboxes;
	size_t next_node, next_bbox;
	std::vector<BVHPrimitive> bag;  // Temporary holding spot for primitives not yet added to the hierarchy

	/**
	 * @brief Tests whether a ray intersects a node or not.
	 */
	inline bool intersect_node(const uint64_t node_i, const Ray& ray, const Vec3& inv_d, const std::array<uint32_t, 3>& d_is_neg, float *near_t, float *far_t) const {
#ifdef GLOBAL_STATS_TOP_LEVEL_BVH_NODE_TESTS
		Global::Stats::bbox_tests++;
#endif
		const Node& node = nodes[node_i];
		const BBox b = lerp_seq<BBox, decltype(bboxes)::const_iterator >(ray.time, bboxes.cbegin() + node.bbox_index, node.ts);
		return b.intersect_ray(ray, inv_d, d_is_neg, near_t, far_t);
	}

	/**
	 * @brief Returns the index of the first child
	 * of the node with the given index.
	 */
	inline size_t child1(const size_t node_i) const {
		return nodes[node_i].child_index;
	}

	/**
	 * @brief Returns the index of the second child
	 * of the node with the given index.
	 */
	inline size_t child2(const size_t node_i) const {
		return nodes[node_i].child_index + 1;
	}

	/**
	 * @brief Returns the index of the sibling
	 * of the node with the given index.
	 */
	inline size_t sibling(const size_t node_i) const {
		if (node_i == nodes[nodes[node_i].parent_index].child_index)
			return node_i + 1;
		else
			return node_i - 1;
	}

public:
	BVH() {
		next_node = 0;
		next_bbox = 0;
	}
	virtual ~BVH();

	// Inherited
	virtual void add_primitives(std::vector<Primitive *> &primitives);
	virtual bool finalize();
	virtual size_t max_primitive_id() const;
	virtual Primitive &get_primitive(size_t id);
	virtual uint get_potential_intersections(const Ray &ray, float tmax, uint max_potential, size_t *ids, void *state);
	virtual size_t size() {
		// TODO
		return 0;
	}
	virtual size_t ray_state_size() {
		return 16;
	}

	virtual BBoxT &bounds();
	virtual bool intersect_ray(const Ray &ray, Intersection *intersection=nullptr);

	unsigned split_primitives(size_t first_prim, size_t last_prim);
	void recursive_build(size_t parent, size_t me, size_t first_prim, size_t last_prim);
};




#endif
