#ifndef BVH2_HPP
#define BVH2_HPP

#include <stdlib.h>
#include <iostream>
#include <vector>
#include <deque>
#include <memory>
#include <tuple>

#include "numtype.h"
#include "global.hpp"

#include "accel.hpp"
#include "bvh.hpp"
#include "object.hpp"
#include "ray.hpp"
#include "bbox.hpp"
#include "utils.hpp"
#include "vector.hpp"




/*
 * A bounding volume hierarchy.
 */
class BVH2: public Accel {
public:
	virtual void build(const Assembly& assembly);
	virtual const std::vector<BBox>& bounds() const {
		return _bounds;
	};
	virtual ~BVH2() {};

	// Traversers need access to private data
	friend class BVH2StreamTraverser;

	struct alignas(16) Node {
	    union {
	        // If the node is a leaf, we don't need the bounds.
	        // If the node is not a leaf, it doesn't have Primitive data.
	        BBox2 bounds {BBox(), BBox()};
	        size_t data_index;
	    };
	    size_t child_index = 0; // When zero, indicates that this is a leaf node
	    uint32_t ts = 0;  // Number of time samples.

	Node() {}

	Node(const Node& n): child_index {n.child_index}, ts {n.ts} {
		bounds = n.bounds;
	}

	// Operators to allow node bounds to be interpolated conveniently
	Node operator+(const Node& b) const {
		Node n;
		n.bounds = bounds + b.bounds;
		return n;
	}

	Node operator*(float f) const {
		Node n;
		n.bounds = bounds * f;
		return n;
	}
	};

private:
	std::vector<Node> nodes;
	std::vector<BBox> _bounds {BBox()};

	enum {
		IS_RIGHT = 1 << 1
	};

	/**
	 * @brief Returns the index of the first child
	 * of the node with the given index.
	 */
	inline size_t child1(const size_t node_i) const {
		return node_i + nodes[node_i].ts;
	}

	/**
	 * @brief Returns the index of the second child
	 * of the node with the given index.
	 */
	inline size_t child2(const size_t node_i) const {
		return nodes[node_i].child_index;
	}

	/**
	 * @brief Returns the number of time samples
	 * of the node with the given index.
	 */
	inline uint32_t time_samples(const size_t node_i) const {
		return nodes[node_i].ts;
	}

	inline bool is_leaf(const size_t node_i) const {
		return nodes[node_i].child_index == 0;
	}
};



/**
 * @brief A breadth-first traverser for BVH2.
 */
class BVH2StreamTraverser: public AccelStreamTraverser<BVH2> {
public:
	virtual ~BVH2StreamTraverser() {}

	virtual void init_accel(const BVH2& accel) {
		bvh = &accel;
	}

	virtual void init_rays(Ray* begin, Ray* end) {
		rays = begin;
		rays_end = end;
		first_call = true;

		// Initialize stack
		if (bvh == nullptr || bvh->nodes.size() == 0) {
			stack_ptr = -1;
		} else {
			stack_ptr = 0;
		}
		node_stack[0] = 0;
		ray_stack[0].first = rays;
		ray_stack[0].second = rays_end;
	}

	virtual std::tuple<Ray*, Ray*, size_t> next_object();

private:
	const BVH2* bvh = nullptr;
	Ray* rays = nullptr;
	Ray* rays_end = nullptr;
	bool first_call = true;

	// Stack data
#define BVH2_STACK_SIZE 64
	int stack_ptr;
	size_t node_stack[BVH2_STACK_SIZE];
	std::pair<Ray*, Ray*> ray_stack[BVH2_STACK_SIZE];

};

#endif // BVH2_HPP