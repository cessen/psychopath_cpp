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

#include <vector>
#include "ray.hpp"
#include "primitive.hpp"
#include "bbox.hpp"
#include "timebox.hpp"
#include "vector.h"
#include <stdlib.h>


class Grid;

struct GridQuantInfo
{
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
        unsigned char bounds[6];
        unsigned char flags;
        
        char pad[7];
};


/*
 * A micropolygon grid.
 */
class Grid: Boundable, Traceable {
    private:
        BBox bbox;
        bool has_bounds;
        
        std::vector<GridBVHNode> bvh_nodes;
        std::vector<GridQuantInfo> quant_info;
        
        void bound_upoly(int first_vert, GridBVHNode *bnodes);
        int recursive_build_bvh(int me, int next_node,
                                int umin, int umax,
                                int vmin, int vmax);
    
    public:
        unsigned short res_u, res_v, res_time; // Grid resolution in vertices
        unsigned short var_count; // Number of variables on each vertex (r, g, b, a, etc.)
        unsigned char time_count;
        
        TimeBox<UVert *> verts; // Grid vertices
        float *vars; // Variables per-vertex, stored ch1,ch2,ch3,ch1,ch2,ch3...
        
        Grid(int ru, int rv, int rt, int vc);
        ~Grid();
        
        bool finalize();
        
        virtual bool intersect_ray(Ray &ray, Intersection *intersection=NULL);
        virtual BBox &bounds();
        
        bool intersect_ray_upoly(Ray &ray, int upoly_i, float *u, float *v, float *t);
        
        void calc_normals();
        
        
        
        /* Returns the approximate size of the grid's data in bytes.
         */
        unsigned int bytes() const
        {
            int vertsize = (res_u * res_v * verts.state_count * sizeof(UVert)) + (verts.state_count * sizeof(UVert *));
            int varsize = (res_u * res_v * var_count * sizeof(float));
            int bvhsize = (((res_u-1) * (res_v-1) * verts.state_count) * 2) * sizeof(GridBVHNode);
            return vertsize + varsize + bvhsize + sizeof(Grid);
        }
};

#endif
