#include "numtype.h"

#include <iostream>
#include <stdlib.h>
#include <math.h>
#include "vector.h"
#include "grid.hpp"
#include "utriangle.hpp"
#include "ray.hpp"
#include "primitive.hpp"
#include "utils.hpp"

#define X_SPLIT 0
#define Y_SPLIT 1
#define Z_SPLIT 2
#define SPLIT_MASK 2
#define SPLIT_NEG 4
#define IS_LEAF 8


inline float32 grid_quant(const float32 &v, const float32 &o, const float32 &f)
{
    return ((v - o) * GRID_BVH_QUANT) / f;
}

inline float32 inv_grid_quant(const float32 &v, const float32 &o, const float32 &f)
{
    return ((v * f) / GRID_BVH_QUANT) + o;
}

inline float32 grid_quantd(const float32 &v, const float32 &o, const float32 &f)
{
    return (v * GRID_BVH_QUANT) / f;
}

Grid::Grid(int32 ru, int32 rv, int32 rt, int32 vc)
{
    res_u = ru;
    res_v = rv;
    var_count = vc;
    time_count = rt;
    
    if(res_u < 2 || res_v < 2 || rt < 1)
    {
        std::cout << "Error: attempt to initialize grid with degenerate resolution: "
                  << res_u << " " << res_v << " " << res_time << std::endl;
        exit(1);
    }
    
    // Initialize vertex list
    verts.init(rt);
    for(int32 i=0; i < time_count; i++)
        verts[i] = new UVert[res_u * res_v];

    if(var_count > 0)
        vars = new float32[res_u * res_v * var_count];
    else
        vars = NULL;
        
    has_bounds = false;
}

Grid::~Grid()
{
    for(int32 i=0; i < time_count; i++)
        delete [] verts[i];
    if(vars)
        delete [] vars;
}


void Grid::calc_normals()
{
    Vec3 p;
    Vec3 vec[4];
    bool v_avail[4] = {false, false, false, false};
    Vec3 n[4];
    bool n_avail[4] = {false, false, false, false};

    int32 upoly_i;
    int32 i2, n_count = 0;
    
    for(int32 time=0; time < time_count; time++)
    {
        for(int32 v=0; v < res_v; v++)
        {
            for(int32 u=0; u < res_u; u++)
            {
                upoly_i = (v * res_u) + u;
                
                // Get the center point
                p = verts[time][upoly_i].p;
                
                // Get the four vectors out from it (or whatever subset exist)
                if(u < (res_u-1)) {
                    vec[0] = verts[time][upoly_i + 1].p - p;
                    v_avail[0] = true;
                }
                
                if(v < (res_v-1)) {
                    vec[1] = verts[time][upoly_i + res_u].p - p;
                    v_avail[1] = true;
                }
                
                if(u > 0) {
                    vec[2] = verts[time][upoly_i - 1].p - p;
                    v_avail[2] = true;
                }
                
                if(v > 0) {
                    vec[3] = verts[time][upoly_i - res_u].p - p;
                    v_avail[3] = true;
                }
                
                // Calculate the normals
                for(int32 i=0; i < 4; i++)
                {
                    i2 = (i + 1) % 4;
                    
                    if(v_avail[i] && v_avail[i2])
                    {
                        n[i] = cross(vec[i], vec[i2]);
                        n_avail[i] = true;
                        n_count++;
                    }
                }
                
                // Average the normals
                verts[time][upoly_i].n = Vec3(0,0,0);
                for(int32 i=0; i < 4; i++)
                {
                    if(n_avail[i])
                        verts[time][upoly_i].n = verts[time][upoly_i].n + n[i];
                }
                verts[time][upoly_i].n = verts[time][upoly_i].n / (float32)(n_count);
                
                // Normalize the normal
                verts[time][upoly_i].n.normalize();
            }
        }
    }
}


bool Grid::intersect_ray_upoly(Ray &ray, int32 upoly_i, float32 *u, float32 *v, float32 *t)
{
    UTriangle tri;
    int32 v1, v2, v3, v4;
    float32 ub, vb, tb;
    bool hit = false;
    bool motion;
    float32 alpha;
    int32 ti1, ti2;

    v1 = upoly_i;
    v2 = upoly_i + 1;
    v3 = upoly_i + res_u + 1;
    v4 = upoly_i + res_u;
    
    // Get the first triangle
    motion = verts.query_time(ray.time, &ti1, &ti2, &alpha);
    if(motion)
    {
        tri.verts[0] = lerp(alpha, verts[ti1][v1].p, verts[ti2][v1].p);
        tri.verts[1] = lerp(alpha, verts[ti1][v2].p, verts[ti2][v2].p);
        tri.verts[2] = lerp(alpha, verts[ti1][v4].p, verts[ti2][v4].p);
    }
    else
    {
        tri.verts[0] = verts[0][v1].p;
        tri.verts[1] = verts[0][v2].p;
        tri.verts[2] = verts[0][v4].p;
    }
        
    // Test first triangle            
    if(tri.intersect_ray_(ray, &tb, &ub, &vb))
    {
        if(tb < *t) {
            *t = tb;
            *u = ub;
            *v = vb;
            hit = true;
        }
    }
    
    // Get the second triangle 
    if(motion)
    {
        tri.verts[0] = lerp(alpha, verts[ti1][v3].p, verts[ti2][v3].p);
    }
    else
    {
        tri.verts[0] = verts[0][v3].p;
    }
            
    // Test second triangle
    if(tri.intersect_ray_(ray, &tb, &ub, &vb))
    {
        if(tb < *t) {
            *t = tb;
            *u = 1.0 - vb;
            *v = 1.0 - ub;
            hit = true;
        }
    }
    
    return hit;
}


/*
 * Utility function to calculate intersections with quantized BVH nodes.
 */
inline bool intersect_grid_bvh_node(uint8 time_count, GridBVHNode *nodes, const Ray &ray,
                                    const uint32 &ia, const float32 &alpha,
                                    GridQuantInfo &qi, std::vector<GridQuantInfo> &qs,
                                    float32 &tmin, float32 &tmax)
{
    //return true;
    Vec3 bounds[2];
    
    // Calculate bounds in time
    if(alpha > 0.0)
    {
        //return true;
        const uint32 ib = ia + 1;
        
        bounds[0].x = lerp(alpha, (float32)nodes[ia].bounds[0], (float32)nodes[ib].bounds[0]);
        bounds[0].y = lerp(alpha, (float32)nodes[ia].bounds[1], (float32)nodes[ib].bounds[1]);
        bounds[0].z = lerp(alpha, (float32)nodes[ia].bounds[2], (float32)nodes[ib].bounds[2]);
        
        /*
        bounds[0].x = lerp(alpha,
                           inv_grid_quant((float32)nodes[ia].bounds[0], qs[ia].offset[0], qs[ia].factor[0]),
                           inv_grid_quant((float32)nodes[ib].bounds[0], qs[ib].offset[0], qs[ib].factor[0]));
        bounds[0].y = lerp(alpha,
                           inv_grid_quant((float32)nodes[ia].bounds[1], qs[ia].offset[1], qs[ia].factor[1]),
                           inv_grid_quant((float32)nodes[ib].bounds[1], qs[ib].offset[1], qs[ib].factor[1]));
        bounds[0].z = lerp(alpha,
                           inv_grid_quant((float32)nodes[ia].bounds[2], qs[ia].offset[2], qs[ia].factor[2]),
                           inv_grid_quant((float32)nodes[ib].bounds[2], qs[ib].offset[2], qs[ib].factor[2]));
        bounds[0].x = grid_quant(bounds[0].x, qi.offset[0], qi.factor[0]);
        bounds[0].y = grid_quant(bounds[0].y, qi.offset[1], qi.factor[1]);
        bounds[0].z = grid_quant(bounds[0].z, qi.offset[2], qi.factor[2]);
        */
        
        bounds[1].x = lerp(alpha, (float32)nodes[ia].bounds[3], (float32)nodes[ib].bounds[3]);
        bounds[1].y = lerp(alpha, (float32)nodes[ia].bounds[4], (float32)nodes[ib].bounds[4]);
        bounds[1].z = lerp(alpha, (float32)nodes[ia].bounds[5], (float32)nodes[ib].bounds[5]);
        
        /*
        bounds[1].x = lerp(alpha,
                           inv_grid_quant((float32)nodes[ia].bounds[3], qs[ia].offset[0], qs[ia].factor[0]),
                           inv_grid_quant((float32)nodes[ib].bounds[3], qs[ib].offset[0], qs[ib].factor[0]));
        bounds[1].y = lerp(alpha,
                           inv_grid_quant((float32)nodes[ia].bounds[4], qs[ia].offset[1], qs[ia].factor[1]),
                           inv_grid_quant((float32)nodes[ib].bounds[4], qs[ib].offset[1], qs[ib].factor[1]));
        bounds[1].z = lerp(alpha,
                           inv_grid_quant((float32)nodes[ia].bounds[5], qs[ia].offset[2], qs[ia].factor[2]),
                           inv_grid_quant((float32)nodes[ib].bounds[5], qs[ib].offset[2], qs[ib].factor[2]));
        bounds[1].x = grid_quant(bounds[1].x, qi.offset[0], qi.factor[0]);
        bounds[1].y = grid_quant(bounds[1].y, qi.offset[1], qi.factor[1]);
        bounds[1].z = grid_quant(bounds[1].z, qi.offset[2], qi.factor[2]);
        */
    }
    else
    {
        bounds[0].x = nodes[ia].bounds[0];
        bounds[0].y = nodes[ia].bounds[1];
        bounds[0].z = nodes[ia].bounds[2];
        
        bounds[1].x = nodes[ia].bounds[3];
        bounds[1].y = nodes[ia].bounds[4];
        bounds[1].z = nodes[ia].bounds[5];
    }
    
    tmin = (bounds[ray.d_is_neg[0]].x - ray.o.x) * ray.inv_d.x;
    tmax = (bounds[1-ray.d_is_neg[0]].x - ray.o.x) * ray.inv_d.x;
    const float32 tymin = (bounds[ray.d_is_neg[1]].y - ray.o.y) * ray.inv_d.y;
    const float32 tymax = (bounds[1-ray.d_is_neg[1]].y - ray.o.y) * ray.inv_d.y;
    const float32 tzmin = (bounds[ray.d_is_neg[2]].z - ray.o.z) * ray.inv_d.z;
    const float32 tzmax = (bounds[1-ray.d_is_neg[2]].z - ray.o.z) * ray.inv_d.z;

    if (tymin > tmin)
        tmin = tymin;
    if (tzmin > tmin)
        tmin = tzmin;
    if (tymax < tmax)
        tmax = tymax;
    if (tzmax < tmax)
        tmax = tzmax;
    
    return (tmin < tmax) && (tmin < ray.max_t) && (tmax > ray.min_t);
}


/* Calculates a ray intersection with the grid.
   t is the distances to the intersection
   Returns false if no intersection, true if there is.
*/
bool Grid::intersect_ray(Ray &rayo, Intersection *intersection)
{
    bool hit = false;
    float32 u=1.0, v=1.0, t=1.0;
    int32 upoly_i=0;
    float32 tnear, tfar;

    // Get the quantization transforms for this ray's time
    uint32 ia = 0;
    float32 alpha = 0.0;
    bool motion = calc_time_interp(time_count, rayo.time, &ia, &alpha);
    
    GridQuantInfo q;
    for(int32 i = 0; i < 3; i++)
    {
        if(motion)
        {
            q.factor[i] = lerp(alpha, quant_info[ia].factor[i], quant_info[ia+1].factor[i]);
            q.offset[i] = lerp(alpha, quant_info[ia].offset[i], quant_info[ia+1].offset[i]);
        }
        else
        {
            q.factor[i] = quant_info[0].factor[i];
            q.offset[i] = quant_info[0].offset[i];
        }
    }
    //float32 tscale = sqrtf((q.factor[0]*q.factor[0]) + (q.factor[1]*q.factor[1]) + (q.factor[2]*q.factor[2]));
    
    // Transform the ray into quant space
    Ray ray;
    ray = rayo;
    for(int32 i = 0; i < 3; i++)
    {
        ray.o[i] = grid_quant(rayo.o[i], q.offset[i], q.factor[i]);
        ray.d[i] = grid_quantd(rayo.d[i], q.offset[i], q.factor[i]);
    }
    
    ray.finalize();
    

    // Traverse the BVH and check for intersections. Yay!
    uint32 todo_offset = 0, node = 0;
    uint32 todo[64];
    
    while(true)
    {
        if(intersect_grid_bvh_node(time_count, &(bvh_nodes[node]), ray, ia, alpha, q, quant_info, tnear, tfar))
        {
            if(bvh_nodes[node].flags & IS_LEAF)
            {
                //Intersect ray with upoly in leaf BVH node
                if(intersect_ray_upoly(rayo, bvh_nodes[node].upoly_index, &u, &v, &(rayo.max_t)))
                {
                    hit = true;
                    upoly_i = bvh_nodes[node].upoly_index;
                    //ray.max_t = rayo.max_t * tscale;
                }
                
                if(todo_offset == 0)
                    break;
                    
                node = todo[--todo_offset];
            }
            else
            {
                // Put far BVH node on todo stack, advance to near node
                todo[todo_offset++] = (bvh_nodes[node].child_index + 1) * time_count;
                node = bvh_nodes[node].child_index * time_count;
            }
        }
        else
        {
            if(todo_offset == 0)
                break;
            node = todo[--todo_offset];
        }
    }
    
    
    // Fill in intersection structure
    if(hit && intersection) {
        float32 l = rayo.d.length();
        Vec3 temp = rayo.d / l;
        temp = temp * t;
        intersection->d = t;
        intersection->p = rayo.o + temp;
        
        // Calculate surface normal at intersection point
        Vec3 n1, n2, n3, n4;
        int32 ia, ib;
        float32 alpha;
        
        if(verts.query_time(ray.time, &ia, &ib, &alpha))
        {   
            // Interpolate normals in time
            n1 = lerp(alpha, verts[ia][upoly_i].n, verts[ib][upoly_i].n);
            n2 = lerp(alpha, verts[ia][upoly_i+1].n, verts[ib][upoly_i+1].n);
            n3 = lerp(alpha, verts[ia][upoly_i+res_u].n, verts[ib][upoly_i+res_u].n);
            n4 = lerp(alpha, verts[ia][upoly_i+res_u+1].n, verts[ib][upoly_i+res_u+1].n);
            n1.normalize();
            n2.normalize();
            n3.normalize();
            n4.normalize();
        }
        else
        {
            n1 = verts[0][upoly_i].n;
            n2 = verts[0][upoly_i+1].n;
            n3 = verts[0][upoly_i+res_u].n;
            n4 = verts[0][upoly_i+res_u+1].n;
        }
        
        // Interpolate normals in UV
        n1 = lerp2d(u, v, n1, n2, n3, n4);
        n1.normalize();
        intersection->n = n1;

        // Normal xyz -> color rgb
        intersection->col = Color((n1.x+1.0)/2, (n1.y+1.0)/2, (n1.z+1.0)/2);
        
        // UV0 -> color rgb
        //intersection->col = Color(u, v, 0.0);
    }
    
    return hit;
}


/* Returns (calculating if necessary) the bounding box of the grid.
*/
BBox &Grid::bounds()
{
    if(!has_bounds)
    {
        bbox.bmin.init(time_count);
        bbox.bmax.init(time_count);
        
        for(int32 time=0; time < time_count; time++)
        {
            bbox.bmin[time] = verts[time][0].p;
            bbox.bmax[time] = verts[time][0].p;
            
            for(int32 i = 0; i < res_u*res_v; i++)
            {
                for(int32 n=0; n < 3; n++)
                {
                    bbox.bmin[time][n] = verts[time][i].p[n] < bbox.bmin[time][n] ? verts[time][i].p[n] : bbox.bmin[time][n];
                    bbox.bmax[time][n] = verts[time][i].p[n] > bbox.bmax[time][n] ? verts[time][i].p[n] : bbox.bmax[time][n];
                }
            }
        }
    }
    
    return bbox;
}



///////////////////////////////////////////////////////////////////////////////
// Methods related to building the BVH.
// We store time-varying nodes as a sequence of nodes (e.g. node1_t1, node1_t2,
// node1_t3, node2_t1, node2_t2, node2_t3, etc.).  Because we know the time
// count on all nodes must be the same, the indexing for this is simple.
///////////////////////////////////////////////////////////////////////////////

/* Bounds a micropolygon of the grid using quantized coordinates.  The polygon
 * is specified by it's first (i.e. upper left) vertex.
 * Bounds are placed into bnodes.
 */
void Grid::bound_upoly(int32 first_vert, GridBVHNode *bnodes)
{
    
    int32 is[4];  // indices to the upoly's four vertices
    is[0] = first_vert;
    is[1] = first_vert + 1;
    is[2] = first_vert + res_u;
    is[3] = first_vert + res_u + 1;
    
    Vec3 bmin, bmax;
    
    for(int32 time = 0; time < time_count; time++)
    {
        // Get the real bounds
        bmin.x = verts[time][is[0]].p.x;
        bmax.x = verts[time][is[0]].p.x;
        bmin.y = verts[time][is[0]].p.y;
        bmax.y = verts[time][is[0]].p.y;
        bmin.z = verts[time][is[0]].p.z;
        bmax.z = verts[time][is[0]].p.z;
        
        for(int32 i = 1; i < 4; i++)
        {
            bmin.x = verts[time][is[i]].p.x < bmin.x ? verts[time][is[i]].p.x : bmin.x;
            bmax.x = verts[time][is[i]].p.x > bmax.x ? verts[time][is[i]].p.x : bmax.x;
            bmin.y = verts[time][is[i]].p.y < bmin.y ? verts[time][is[i]].p.y : bmin.y;
            bmax.y = verts[time][is[i]].p.y > bmax.y ? verts[time][is[i]].p.y : bmax.y;
            bmin.z = verts[time][is[i]].p.z < bmin.z ? verts[time][is[i]].p.z : bmin.z;
            bmax.z = verts[time][is[i]].p.z > bmax.z ? verts[time][is[i]].p.z : bmax.z;
        }
        
        // Quantize the bounds
        for(int32 i = 0; i < 3; i++)
        {
            bnodes[time].bounds[i] = grid_quant(bmin[i], quant_info[time].offset[i], quant_info[time].factor[i]);
            bnodes[time].bounds[i+3] = grid_quant(bmax[i], quant_info[time].offset[i], quant_info[time].factor[i]);
            if(bnodes[time].bounds[i+3] < GRID_BVH_QUANT)
                bnodes[time].bounds[i+3] += 1;
        }
    }
}


/*
 * Bounds the grid, calculates quantization factors, and builds bvh.
 * Should be run only after all displacements etc. have been done.
 */
bool Grid::finalize()
{
    // Calculate grid bounds
    has_bounds = false;
    bounds();

    // Calculate quantization information
    float32 factor[3] = {0.0, 0.0, 0.0};
    quant_info.resize(time_count);
    for(int32 time = 0; time < time_count; time++)
    {
        for(int32 i = 0; i < 3; i++)
        {
            quant_info[time].offset[i] = bbox.bmin[time][i];
            
            float32 f = bbox.bmax[time][i] - bbox.bmin[time][i];
            if(f < 0.000001)
                f = 0.000001;
            
            if(f > factor[i])
                factor[i] = f;
        }
    }
    for(int32 time=0; time < time_count; time++)
    {
        quant_info[time].factor[0] = factor[0];
        quant_info[time].factor[1] = factor[1];
        quant_info[time].factor[2] = factor[2];
    }

    // Calculate bvh
    bvh_nodes.resize((res_u-1) * (res_v-1) * time_count * 2);
    recursive_build_bvh(0, 1, 0, res_u-2, 0, res_v-2);
    
    return true;
}


/*
 * Recursive building of the grid's bvh.
 */
int32 Grid::recursive_build_bvh(int32 me, int32 next_node,
                   int32 umin, int32 umax,
                   int32 vmin, int32 vmax)
{
    bvh_nodes[me].flags = 0;

    // Leaf node?
    if(umin == umax && vmin == vmax)
    {
        bvh_nodes[me].flags |= IS_LEAF;
        bvh_nodes[me].upoly_index = (vmin * res_u) + umin;
        bound_upoly(bvh_nodes[me].upoly_index, &bvh_nodes[me]);
        
        return next_node;
    }
    
    // Not a leaf
    bvh_nodes[me].child_index = next_node;
    
    // Create child nodes
    int32 next_i = next_node + 2;
    int32 child1i = next_node * time_count;
    int32 child2i = (next_node + 1) * time_count;
    if((umax-umin) > (vmax-vmin))
    {
        // Split on U
        int32 u = umin + ((umax-umin) / 2);
        next_i = recursive_build_bvh(child1i, next_i,
                                     umin, u,
                                     vmin, vmax);
        next_i = recursive_build_bvh(child2i, next_i,
                                     u+1, umax,
                                     vmin, vmax);
    }
    else
    {
        // Split on V
        int32 v = vmin + ((vmax-vmin) / 2);
        next_i = recursive_build_bvh(child1i, next_i,
                                     umin, umax,
                                     vmin, v);
        next_i = recursive_build_bvh(child2i, next_i,
                                     umin, umax,
                                     v+1, vmax);
    }
    
    // Calculate bounds
    for(uint32 time = 0; time < time_count; time++)
    {
        //std::cout << "Time " << time << ": ";
        for(int32 i=0; i < 3; i++)
        {
            bvh_nodes[me+time].bounds[i] = bvh_nodes[child1i+time].bounds[i];
            bvh_nodes[me+time].bounds[i+3] = bvh_nodes[child1i+time].bounds[i+3];
            
            if(bvh_nodes[me+time].bounds[i] > bvh_nodes[child2i+time].bounds[i])
                bvh_nodes[me+time].bounds[i] = bvh_nodes[child2i+time].bounds[i];
            if(bvh_nodes[me+time].bounds[i+3] < bvh_nodes[child2i+time].bounds[i+3])
                bvh_nodes[me+time].bounds[i+3] = bvh_nodes[child2i+time].bounds[i+3];
        }
    }
    
    // Return the next available node index;
    return next_i;
}

