#ifndef GRID_HPP
#define GRID_HPP
/*
 * This file and grid.cpp define micropolygon grids.
 * They come with their own special spatial acceleration structure,
 * and they know how to intersect rays with themselves.
 *
 * TODO:
 * - Quantized bounds for bvh nodes.
 */

#include "numtype.h"

#include <vector>
#include "ray.hpp"
#include "primitive.hpp"
#include "bbox.hpp"
#include "timebox.hpp"
#include "vector.h"
#include <stdlib.h>


class Grid;

struct GridQuantInfo {
	Vec3 offset;
	Vec3 factor;
};

/*
 * A single vertex of a micropolygon.
 */
class UVert
{
public:
	Vec3 p; // Position
	Vec3 n; // Normal
};


#define GRID_BVH_QUANT 255

/*
 * A node of a grid BVH.
 */
class GridBVHNode
{
public:
	union {
		unsigned short child_index;
		unsigned short upoly_index;
	};

	/* Bounding boxes of the node, layed out as minx, miny, minz,
	   maxx, maxy, maxz.
	   Bounds are quantized to bytes, relative to the over-all grid bounding
	   box at each time step.
	 */
	uint8 bounds[6];
	uint8 flags;

	char pad[7];
};


/*
 * A micropolygon grid.
 */
class Grid: Boundable, Traceable
{
private:
	BBox bbox;
	bool has_bounds;

	std::vector<GridBVHNode> bvh_nodes;
	std::vector<GridQuantInfo> quant_info;

	void bound_upoly(int32 first_vert, GridBVHNode *bnodes);
	int32 recursive_build_bvh(int32 me, int32 next_node,
	                          int32 umin, int32 umax,
	                          int32 vmin, int32 vmax);

public:
	unsigned short res_u, res_v, res_time; // Grid resolution in vertices
	unsigned short var_count; // Number of variables on each vertex (r, g, b, a, etc.)
	uint8 time_count;

	TimeBox<UVert *> verts; // Grid vertices
	float32 *vars; // Variables per-vertex, stored ch1,ch2,ch3,ch1,ch2,ch3...

	Grid(int32 ru, int32 rv, int32 rt, int32 vc);
	~Grid();

	bool finalize();

	virtual bool intersect_ray(Ray &ray, Intersection *intersection=NULL);
	virtual BBox &bounds();

	bool intersect_ray_upoly(Ray &ray, int32 upoly_i, float32 *u, float32 *v, float32 *t);

	void calc_normals();



	/* Returns the approximate size of the grid's data in bytes.
	 */
	uint32 bytes() const {
		int32 vertsize = (res_u * res_v * verts.state_count * sizeof(UVert)) + (verts.state_count * sizeof(UVert *));
		int32 varsize = (res_u * res_v * var_count * sizeof(float32));
		int32 bvhsize = (((res_u-1) * (res_v-1) * verts.state_count) * 2) * sizeof(GridBVHNode);
		return vertsize + varsize + bvhsize + sizeof(Grid);
	}
};

#endif
