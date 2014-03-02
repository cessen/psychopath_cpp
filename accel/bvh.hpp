#ifndef BVH_HPP
#define BVH_HPP

#include "numtype.h"
#include "global.hpp"

#include <stdlib.h>
#include <iostream>
#include <vector>
#include <memory>
#include "object.hpp"
#include "accel.hpp"
#include "ray.hpp"
#include "bbox.hpp"
#include "utils.hpp"
#include "vector.hpp"
#include "chunked_array.hpp"






/*
 * A bounding volume hierarchy.
 */
class BVH: public Accel
{
public:
	virtual ~BVH() {};
	virtual void build(std::vector<std::unique_ptr<Object>>* objects);

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
			size_t child_index;
			Object *data = nullptr;
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
	 * Used to store objects that have yet to be
	 * inserted into the hierarchy.
	 * Contains the time 0.5 bounds of the object and it's centroid.
	 */
	struct BVHPrimitive {
		Object *data;
		Vec3 bmin, bmax, c;

		BVHPrimitive() {
			data = nullptr;
		}

		BVHPrimitive(Object *prim) {
			init(prim);
		}

		void init(Object *prim) {
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
		const BBox b = lerp_seq<BBox, decltype(bboxes)::const_iterator >(ray.time, bboxes.cbegin() + node.bbox_index, node.ts);
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

	virtual void init_rays(const WorldRay* begin, const WorldRay* end) {
		w_rays = begin;
		w_rays_end = end;
		rays.resize(std::distance(begin, end));

		for (size_t i = 0; i < rays.size(); ++i) {
			rays[i] = w_rays[i].to_ray();
			rays[i].id = i;
		}

		// Initialize stack
		stack_ptr = 0;
		node_stack[0] = 0;
		ray_stack[0].first = rays.begin();
		ray_stack[0].second = rays.end();
	}

	virtual std::tuple<Ray*, Ray*, Object*> next_object();

private:
	const BVH* bvh = nullptr;
	const WorldRay* w_rays = nullptr;
	const WorldRay* w_rays_end = nullptr;
	std::vector<Ray> rays;

	// Stack data
#define BVHST_STACK_SIZE 64
	int stack_ptr;
	size_t node_stack[BVHST_STACK_SIZE];
	std::pair<std::vector<Ray>::iterator, std::vector<Ray>::iterator> ray_stack[BVHST_STACK_SIZE];

};


#endif // BVH_HPP
