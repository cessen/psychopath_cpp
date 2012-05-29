#ifndef BVH_H
#define BVH_H

#include "numtype.h"

#include <stdlib.h>
#include <iostream>
#include <vector>
#include "primitive.hpp"
#include "aggregate.hpp"
#include "ray.hpp"
#include "bbox.hpp"
#include "utils.hpp"
#include "vector.h"

#define BVH_CHUNK_SIZE 1024


struct BucketInfo {
	BucketInfo() {
		count = 0;
	}
	int32 count;
	BBox bb;
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
		data = NULL;
	}

	void init(Primitive *prim) {
		data = prim;

		// Get bounds at time 0.5
		int32 ia, ib;
		float32 alpha;
		if (data->bounds().bmin.query_time(0.5, &ia, &ib, &alpha)) {
			bmin = lerp(0.5, data->bounds().bmin[ia], data->bounds().bmin[ib]);
			bmax = lerp(0.5, data->bounds().bmax[ia], data->bounds().bmax[ib]);
		} else {
			bmin = data->bounds().bmin[0];
			bmax = data->bounds().bmax[0];
		}

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
	BBox b;
	union {
		uint32 child_index;
		Primitive *data;
	};

	uint8 flags;

	BVHNode() {
		child_index = 0;
		data = NULL;
		flags = 0;
	}
};


/*
 * A collection of BVHNodes.  Like a resizable array.
 * Allocates nodes in chunks, for less RAM shuffling.
 */
class BVHNodes
{
public:
	std::vector<BVHNode *> nodes;
	uint32 num_nodes;

	BVHNodes() {
		num_nodes = 0;
	}

	~BVHNodes() {
		int32 s = nodes.size();
		for (int32 i=0; i < s; i++) {
			delete [] nodes[i];
		}
	}

	BVHNode &operator[](const int32 &i) {
		return (nodes[i/BVH_CHUNK_SIZE])[i%BVH_CHUNK_SIZE];
	}

	const BVHNode &operator[](const int32 &i) const {
		return (nodes[i/BVH_CHUNK_SIZE])[i%BVH_CHUNK_SIZE];
	}

	uint32 size() const {
		return num_nodes;
	}

	void add_chunk() {
		int32 s = nodes.size();
		nodes.resize(s+1);
		nodes[s] = new BVHNode[BVH_CHUNK_SIZE];
		num_nodes = BVH_CHUNK_SIZE * (s+1);
	}
};



/*
 * A bounding volume hierarchy.
 */
class BVH: public Aggregate
{
private:
	BBox bbox;
	BVHNodes nodes;
	uint32 next_node;
	std::vector<BVHPrimitive> bag;  // Temporary holding spot for primitives not yet added to the hierarchy

public:
	BVH() {
		next_node = 0;
	}
	virtual ~BVH();

	// Inherited
	virtual void add_primitives(std::vector<Primitive *> &primitives);
	virtual bool finalize();

	virtual BBox &bounds();
	virtual bool intersect_ray(Ray &ray, Intersection *intersection=NULL);

	unsigned split_primitives(uint32 first_prim, uint32 last_prim, int32 *axis=NULL);
	void recursive_build(uint32 me, uint32 first_prim, uint32 last_prim);
};




#endif
