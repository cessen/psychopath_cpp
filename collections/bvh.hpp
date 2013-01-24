#ifndef BVH_HPP
#define BVH_HPP

#include "numtype.h"

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


#define BVH_NODE_CHUNK_SIZE 4096

struct BucketInfo {
	BucketInfo() {
		count = 0;
	}
	uint_i count;
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
 * A node of a bounding volume hierarchy.
 * Contains a bounding box, a flag for whether
 * it's a leaf or not, a pointer to its first
 * child, and it's data if it's a leaf.
 */
class BVHNode
{
public:
	uint_i bbox_index;
	union {
		uint_i child_index;
		Primitive *data;
	};
	uint_i parent_index;
	uint16 ts; // Time sample count
	uint16 flags;

	BVHNode() {
		bbox_index = 0;
		child_index = 0;
		data = nullptr;
		ts = 0;
		flags = 0;
	}
};



/*
 * A bounding volume hierarchy.
 */
class BVH: public Collection
{
private:
	BBoxT bbox;
	ChunkedArray<BVHNode, BVH_NODE_CHUNK_SIZE> nodes;
	ChunkedArray<BBox, BVH_NODE_CHUNK_SIZE> bboxes;
	uint_i next_node, next_bbox;
	std::vector<BVHPrimitive> bag;  // Temporary holding spot for primitives not yet added to the hierarchy

	/**
	 * @brief Tests whether a ray intersects a node or not.
	 */
	bool intersect_node(uint_i node, const Ray &ray, float32 *near_t, float32 *far_t) {
		if (nodes[node].ts == 1) {
			return bboxes[nodes[node].bbox_index].intersect_ray(ray, near_t, far_t);
		} else {
			const BBox b = lerp_seq<BBox, ChunkedArrayIterator<BBox, BVH_NODE_CHUNK_SIZE> >(ray.time, bboxes.get_iterator(nodes[node].bbox_index), nodes[node].ts);
			return b.intersect_ray(ray, near_t, far_t);
		}
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
	virtual uint_i max_primitive_id() const;
	virtual Primitive &get_primitive(uint_i id);
	virtual uint get_potential_intersections(const Ray &ray, uint max_potential, uint_i *ids, void *state);
	virtual uint_i size() {
		// TODO
		return 0;
	}
	virtual size_t ray_state_size() {
		return 16;
	}

	virtual BBoxT &bounds();
	virtual bool intersect_ray(Ray &ray, Intersection *intersection=nullptr);

	unsigned split_primitives(uint_i first_prim, uint_i last_prim, int32 *axis=nullptr);
	void recursive_build(uint_i parent, uint_i me, uint_i first_prim, uint_i last_prim);
};




#endif
