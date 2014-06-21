#ifndef BVH4_HPP
#define BVH4_HPP

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
class BVH4: public Accel
{
public:
	virtual void build(const Assembly& assembly);
	virtual const std::vector<BBox>& bounds() const {
		return _bounds;
	};
	virtual ~BVH4() {};

	// Traversers need access to private data
	friend class BVH4StreamTraverser;

	struct alignas(16) Node {
	    union {
	        // If the node is a leaf, we don't need the bounds.
	        // If the node is not a leaf, it doesn't have Primitive data.
	        BBox4 bounds {BBox(), BBox(), BBox(), BBox()};
	        size_t data_index;
	    };
	    size_t child_indices[3] = {0,0,0}; // Indices of children 2, 3, and 4. (Child 1's index is implicit.)
	    // When first element is 0, indicates that this is a leaf node,
	    // because a non-leaf node needs at least two children.  When the
	    // second and/or third elements are zero, indicates there is no
	    // third or fourth child, respectively.
	    uint32_t ts = 0;  // Number of time samples.

	Node() {}

//		Node(const Node& n): child_indices {n.child_indices}, ts {n.ts} {
//			bounds = n.bounds;
//		}

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
	    IS_SKIP = 1 << 8,
	    IS_2ND  = 1 << 9,
	    IS_3RD  = 1 << 10,
	    IS_4TH  = 1 << 11
	};

	/**
	 * @brief Returns the index of the nth (0-3) child
	 * of the node with the given index.
	 */
	inline size_t child(const size_t node_i, const int n) const {
		if (n == 0)
			return node_i + nodes[node_i].ts;
		else
			return nodes[node_i].child_indices[n-1];
	}

	/**
	 * @brief Returns the number of time samples
	 * of the node with the given index.
	 */
	inline uint32_t time_samples(const size_t node_i) const {
		return nodes[node_i].ts;
	}

	/**
	 * @brief Returns whether the node with the given index is a
	 * leaf node or not.
	 */
	inline bool is_leaf(const size_t node_i) const {
		return (nodes[node_i].child_indices[0] == 0);
	}

	inline int child_count(const size_t node_i) const {
		if (nodes[node_i].child_indices[1] == 0) {
			return 2;
		} else if (nodes[node_i].child_indices[2] == 0) {
			return 3;
		} else {
			return 4;
		}
	}
};




/**
 * @brief A breadth-first traverser for BVH4.
 */
class BVH4StreamTraverser: public AccelStreamTraverser<BVH4>
{
public:
	virtual ~BVH4StreamTraverser() {}

	virtual void init_accel(const BVH4& accel) {
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
	const BVH4* bvh = nullptr;
	Ray* rays = nullptr;
	Ray* rays_end = nullptr;
	bool first_call = true;

	// Stack data
#define BVH4_STACK_SIZE 64
	int stack_ptr;
	size_t node_stack[BVH4_STACK_SIZE];
	std::pair<Ray*, Ray*> ray_stack[BVH4_STACK_SIZE];

};


#endif // BVH4_HPP