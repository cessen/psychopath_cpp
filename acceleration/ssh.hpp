#ifndef SSH_HPP
#define SSH_HPP

/*
 * A "Single Slab Hierarchy" ray tracing acceleration structure.
 * See "Ray Tracing with the Single Slab Hierarchy"
 * by Eisemann et al. for more information.
 */

#include "numtype.h"

#include <stdlib.h>
#include <iostream>
#include <vector>
#include "primitive.hpp"
#include "aggregate.hpp"
#include "ray.hpp"
#include "bbox.hpp"
#include "utils.hpp"

#define SSH_CHUNK_SIZE 1024



/*
 * Used to store primitives that have yet to be
 * inserted into the hierarchy.
 * Contains the bound's centroid at time 0.5 and
 * a pointer to the primitive.
 */
class SSHPrimitive
{
public:
	Primitive *data;
	Vec c;

	SSHPrimitive() {
		data = NULL;
	}

	void init(Primitive *prim) {
		Vec bmin, bmax;

		// Store primitive pointer and get bounds
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
 * A node of the single slab hierarchy.
 */
class SSHNode
{
public:
	TimeBox<float32> plane;  // Potentially multiple planes for multiple time samples

	union {
		uint32 child_index;
		Primitive *data;
	};

	unsigned char flags;

	SSHNode() {
		child_index = 0;
		data = NULL;
		flags = 0;
	}
};


/*
 * A collection of SSHNodes.  Like a resizable array.
 * Allocates nodes in chunks, for less RAM shuffling.
 */
class SSHNodes
{
public:
	std::vector<SSHNode *> nodes;
	uint32 num_nodes;

	SSHNodes() {
		num_nodes = 0;
	}

	~SSHNodes() {
		int32 s = nodes.size();
		for (int32 i=0; i < s; i++) {
			delete [] nodes[i];
		}
	}

	SSHNode &operator[](int32 i) {
		if (SSH_CHUNK_SIZE > 1)
			return (nodes[i/SSH_CHUNK_SIZE])[i%SSH_CHUNK_SIZE];
		else
			return *nodes[i];
	}

	uint32 size() {
		return num_nodes;
	}

	void add_chunk() {
		int32 s = nodes.size();
		nodes.resize(s+1);
		nodes[s] = new SSHNode[SSH_CHUNK_SIZE];
		num_nodes = SSH_CHUNK_SIZE * (s+1);
	}
};



/*
 * A bounding volume hierarchy.
 */
class SSH: public Aggregate
{
private:
	BBox bbox;
	SSHNodes nodes;
	uint32 next_node;
	std::vector<SSHPrimitive> bag;  // Temporary holding spot for primitives not yet added to the hierarchy

public:
	SSH() {
		next_node = 0;
	}
	virtual ~SSH();

	// Inherited
	virtual void add_primitives(std::vector<Primitive *> &primitives);
	virtual bool finalize();

	virtual BBox &bounds();
	virtual bool intersect_ray(Ray &ray, Intersection *intersection=NULL);

	unsigned split_primitives(uint32 first_prim, uint32 last_prim, int32 *axis=NULL);
	void recursive_build(uint32 me, uint32 first_prim, uint32 last_prim, BBox &parent_bounds);
};




#endif
