#ifndef BVH4_HPP
#define BVH4_HPP

#include "numtype.h"
#include "global.hpp"

#include <stdlib.h>
#include <iostream>
#include <vector>
#include <deque>
#include <memory>
#include <tuple>
#include <x86intrin.h>
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
class BVH4: public Collection
{
public:
	virtual ~BVH4() {};

	virtual void add_primitives(std::vector<std::unique_ptr<Primitive>>* primitives);
	virtual bool finalize();
	virtual size_t max_primitive_id() const;
	virtual Primitive &get_primitive(size_t id);
	virtual uint get_potential_intersections(const Ray &ray, float tmax, uint max_potential, size_t *ids, void *state);
	virtual size_t ray_state_size() {
		return 16;
	}

	struct Node {
		uint64_t parent_index_and_ts = 0;  // Stores both the parent index and the number of time samples.
		size_t child_indices[3] = {0,0,0}; // When first element is 0, indicates that this is a leaf node,
		// because a non-leaf node needs at least two children.  When the
		// second and/or third elements are zero, indicates there is no
		// third or fourth child, respectively.
		union {
			// If the node is a leaf, we don't need the bounds.
			// If the node is not a leaf, it doesn't have Primitive data.
			BBox4 bounds {BBox(), BBox(), BBox(), BBox()};
			Primitive *data;
		};

		Node() {}

		Node(const Node& n): parent_index_and_ts {n.parent_index_and_ts} {
			for (int i = 0; i < 3; ++i)
				child_indices[i] = n.child_indices[i];
			bounds = n.bounds;
		}

		void set_time_samples(const uint64_t ts) {
			parent_index_and_ts &= 0x0000FFFFFFFFFFFF;
			parent_index_and_ts |= (ts << 48);
		}

		void set_parent_index(const uint64_t par_i) {
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
	 * @brief Returns the index of the nth (0-3) child
	 * of the node with the given index.
	 */
	inline size_t child(const size_t node_i, const int n) const {
		if (n == 0)
			return node_i + time_samples(node_i);
		else
			return nodes[node_i].child_indices[n-1];
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
	 * @brief Returns the index of the nth (0-3) sibling
	 * of the node with the given index.  "n" is absolute,
	 * not relative.  So passing n=0 will return the index
	 * of the first child of the parent, regardless of the
	 * node index passed in.
	 */
	inline size_t sibling(const size_t node_i, const int n) const {
		return child(parent(node_i), n);
	}

	/**
	 * @brief Returns whether the node with the given index is a
	 * leaf node or not.
	 */
	inline bool is_leaf(const size_t node_i) const {
		return (nodes[node_i].child_indices[0] == 0);
	}



	size_t split_primitives(size_t first_prim, size_t last_prim);
	size_t recursive_build(size_t parent, size_t first_prim, size_t last_prim);
	void pack();
};




#endif // BVH4_HPP
