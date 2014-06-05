#ifndef BVH_HPP
#define BVH_HPP

#include "numtype.h"
#include "global.hpp"

#include "object.hpp"
#include "accel.hpp"
#include "ray.hpp"
#include "bbox.hpp"
#include "utils.hpp"
#include "vector.hpp"
#include "chunked_array.hpp"
#include "scene_graph.hpp"

#include <stdlib.h>
#include <iostream>
#include <vector>
#include <memory>




/*
 * A bounding volume hierarchy.
 */
class BVH: public Accel
{
	std::vector<BBox> _bounds {BBox()}; // TODO: build this properly

public:
	virtual ~BVH() {};
	virtual void build(const Assembly& assembly);
	virtual const std::vector<BBox>& bounds() const {
		return _bounds;
	};

	// Traversers need access to private data
	friend class BVHStreamTraverser;


	enum {
	    IS_LEAF = 1 << 0
	};

	/*
	 * A node of a bounding volume hierarchy.
	 * Contains a bounding box, a flag for whether
	 * it's a leaf or not, a pointer to its first
	 * child, and it's data if it's a leaf.
	 */
	struct Node {
		size_t bbox_index = 0;
		union {
			size_t child_index = 0;
			size_t data_index;
		};
		size_t parent_index = 0;
		uint16_t ts = 0; // Time sample count
		uint16_t flags = 0;
	};

	/*
	 * Used to store objects that have yet to be
	 * inserted into the hierarchy.
	 * Contains the time 0.5 bounds of the object and it's centroid.
	 */
	struct BVHPrimitive {
		size_t instance_index;
		Vec3 bmin, bmax, c;
	};

private:
	const Assembly* assembly; // Set during build()
	//std::vector<BBox> bbox;
	std::vector<Node> nodes;
	std::vector<BBox> bboxes;
	std::vector<BVHPrimitive> bag;  // Temporary holding spot for objects not yet added to the hierarchy

	bool finalize();

	/**
	 * @brief Tests whether a ray intersects a node or not.
	 */
	inline bool intersect_node(const uint64_t node_i, const Ray& ray, float *near_t, float *far_t) const {
#ifdef GLOBAL_STATS_TOP_LEVEL_BVH_NODE_TESTS
		Global::Stats::top_level_bvh_node_tests++;
#endif
		const Node& node = nodes[node_i];
		const BBox b = lerp_seq(ray.time, bboxes.cbegin() + node.bbox_index, bboxes.cbegin() + node.bbox_index + node.ts);
		return b.intersect_ray(ray, near_t, far_t, ray.max_t);
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

	inline bool is_leaf(const size_t node_i) const {
		return nodes[node_i].flags & IS_LEAF;
	}

	size_t split_primitives(size_t first_prim, size_t last_prim);
	size_t recursive_build(size_t parent, size_t first_prim, size_t last_prim);
};



/**
 * @brief A breadth-first traverser for BVH.
 */
class BVHStreamTraverser: public AccelStreamTraverser<BVH>
{
public:
	virtual ~BVHStreamTraverser() {}

	virtual void init_accel(const BVH& accel) {
		bvh = &accel;
	}

	virtual void init_rays(Ray* begin, Ray* end) {
		rays = begin;
		rays_end = end;

		// Initialize stack
		stack_ptr = 0;
		node_stack[0] = 0;
		ray_stack[0].first = rays;
		ray_stack[0].second = rays_end;
	}

	virtual std::tuple<Ray*, Ray*, size_t> next_object();

private:
	const BVH* bvh = nullptr;
	Ray* rays = nullptr;
	Ray* rays_end = nullptr;

	// Stack data
#define BVHST_STACK_SIZE 64
	int stack_ptr;
	size_t node_stack[BVHST_STACK_SIZE];
	std::pair<Ray*, Ray*> ray_stack[BVHST_STACK_SIZE];

};


#endif // BVH_HPP
