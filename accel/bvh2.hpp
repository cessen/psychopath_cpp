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

#include "accel.hpp"
#include "object.hpp"
#include "ray.hpp"
#include "bbox.hpp"
#include "utils.hpp"
#include "vector.hpp"
#include "chunked_array.hpp"
#include "scene_graph.hpp"




/*
 * A bounding volume hierarchy.
 */
class BVH2: public Accel
{
public:
	virtual ~BVH2() {};

	virtual void build(const Assembly& assembly);

	struct Node {
		uint64_t parent_index_and_ts = 0;  // Stores both the parent index and the number of time samples.
		size_t child_index = 0; // When zero, indicates that this is a leaf node
		union {
			// If the node is a leaf, we don't need the bounds.
			// If the node is not a leaf, it doesn't have Primitive data.
			BBox2 bounds {BBox(), BBox()};
			Primitive *data;
		};

		Node() {}

		Node(const Node& n): parent_index_and_ts {n.parent_index_and_ts}, child_index {n.child_index} {
			bounds = n.bounds;
		}

		void set_time_samples(const size_t ts) {
			parent_index_and_ts &= 0x0000FFFFFFFFFFFF;
			parent_index_and_ts |= (ts << 48);
		}

		void set_parent_index(const size_t par_i) {
			parent_index_and_ts &= 0xFFFF000000000000;
			parent_index_and_ts |= (par_i & 0x0000FFFFFFFFFFFF);
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
	 * @brief Returns the index of the first child
	 * of the node with the given index.
	 */
	inline size_t child1(const size_t node_i) const {
		return node_i + time_samples(node_i);
	}

	/**
	 * @brief Returns the index of the second child
	 * of the node with the given index.
	 */
	inline size_t child2(const size_t node_i) const {
		return nodes[node_i].child_index;
	}

	/**
	 * @brief Returns the index of the parent
	 * of the node with the given index.
	 */
	inline size_t parent(const size_t node_i) const {
		return nodes[node_i].parent_index_and_ts & 0x0000FFFFFFFFFFFF;
	}

	/**
	 * @brief Returns the number of time samples
	 * of the node with the given index.
	 */
	inline uint32_t time_samples(const size_t node_i) const {
		return (nodes[node_i].parent_index_and_ts >> 48) & 0x000000000000FFFF;
	}

	/**
	 * @brief Returns the index of the sibling
	 * of the node with the given index.
	 */
	inline size_t sibling(const size_t node_i) const {
		const size_t par_i = parent(node_i);
		if (node_i == child2(par_i))
			return child1(par_i);
		else
			return child2(par_i);
	}



	size_t split_primitives(size_t first_prim, size_t last_prim);
	size_t recursive_build(size_t parent, size_t first_prim, size_t last_prim);
	void pack();
};




#endif // BVH2_HPP