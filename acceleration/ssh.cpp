#include <iostream>
#include <algorithm>
#include "ray.hpp"
#include "ssh.h"
#include <cmath>


#define X_BOUND 0
#define Y_BOUND 1
#define Z_BOUND 2
#define BOUND_MASK 2
#define NEG_BOUND 4

#define IS_LEAF 8

#define X_SPLIT 16
#define Y_SPLIT 32
#define Z_SPLIT 48
#define SPLIT_MASK 48




SSH::~SSH()
{
    for(unsigned int i=0; i < next_node; i++)
    {
        if(nodes[i].flags & IS_LEAF)
            delete nodes[i].data;
    }
}

void SSH::add_primitives(std::vector<Primitive *> &primitives)
{
    int start = bag.size();
    int added = primitives.size();
    bag.resize(start + added);
    
    for(int i=0; i < added; i++)
    {
        bag[start + i].init(primitives[i]);
    }
}

bool SSH::finalize()
{
    // Calculate over-all SSH bounding box
    bbox.copy(bag[0].data->bounds());
    for(unsigned int i=1; i < bag.size(); i++)
        bbox.merge(bag[i].data->bounds());
    
    // Generate SSH
    next_node = 1;
    recursive_build(0, 0, bag.size()-1, bbox);
    bag.resize(0);
    return true;
}


struct CompareToMid {
    int dim;
    float mid;
    
    CompareToMid(int d, float m)
    {
        dim = d;
        mid = m;
    }
    
    bool operator()(SSHPrimitive &a) const {
        return a.c[dim] < mid;
    }
};

struct CompareDim {
    int dim;
    
    CompareDim(int d)
    {
        dim = d;
    }
    
    bool operator()(SSHPrimitive &a, SSHPrimitive &b) const {
        return a.c[dim] < b.c[dim];
    }
};



/*
 * Determines the split of the primitives in bag starting
 * at first and ending at last.  May reorder that section of the
 * list.  Used in recursive_build for SSH construction.
 * Returns the split index (last index of the first group).
 */
unsigned int SSH::split_primitives(unsigned int first_prim, unsigned int last_prim, int *axis)
{
    //std::cout << "First: " << first_prim << " Last: " << last_prim << std::endl;

    // Find the minimum and maximum centroid values on each axis
    Vec min, max;
    min = bag[first_prim].c;
    max = bag[first_prim].c;
    for(unsigned int i = first_prim+1; i <= last_prim; i++)
    {
        for(int d = 0; d < 3; d++)
        {
            min[d] = min[d] < bag[i].c[d] ? min[d] : bag[i].c[d];  
            max[d] = max[d] > bag[i].c[d] ? max[d] : bag[i].c[d];
        }
    }
    
    // Find the axis with the maximum extent
    int max_axis = 0;
    if((max.y - min.y) > (max.x - min.x))
        max_axis = 1;
    if((max.z - min.z) > (max.y - min.y))
        max_axis = 2;
    
    if(axis)
        *axis = max_axis;
        
    // Sort and split the list
    float pmid = .5f * (min[max_axis] + max[max_axis]);
    SSHPrimitive *mid_ptr = std::partition(&bag[first_prim],
                                           (&bag[last_prim])+1,
                                           CompareToMid(max_axis, pmid));
    
    unsigned int split = (mid_ptr - &bag.front());
    if(split > 0)
        split--;
        
    if(split < first_prim)
        split = first_prim;
    
    //std::cout << "First: " << first_prim << " Split: " << split << std::endl;
    
    return split;
}



/*
 * Recursively builds the SSH starting at the given node with the given
 * first and last primitive indices (in bag).
 */
void SSH::recursive_build(unsigned int me, unsigned int first_prim, unsigned int last_prim, BBox &parent_bounds)
{
    // Need to allocate more node space?
    if(me >= nodes.size())
        nodes.add_chunk();
    
    std::cout << "\nNode: " << me << std::endl;
    std::cout << "First: " << first_prim << " Last: " << last_prim << std::endl; std::cout.flush();    
    
    nodes[me].flags = 0;
    
    // Calculate collective primitive bounds
    BBox my_bounds;
    my_bounds.copy(bag[first_prim].data->bounds());
    for(unsigned int i=first_prim+1; i <= last_prim; i++)
        my_bounds.merge(bag[i].data->bounds());
    
    nodes[me].plane.init(my_bounds.bmin.state_count);
    
    // Calculate bounding plane
    int plane_axis=0;
    bool plane_neg=false;
    float area = 999999999999999999999999.0;
    for(int d=0; d < 3; d++)
    {
        BBox temp;
        float temp_area;
        
        // Contents on positive side
        temp.copy(parent_bounds);
        temp.bmin[0][d] = my_bounds.bmin[0][d];
        temp_area = temp.surface_area();
        if(temp_area < area)
        {
            area = temp_area;
            plane_axis = d;
            plane_neg = false;
            for(int j=0; j < nodes[me].plane.state_count; j++)
                nodes[me].plane[j] = my_bounds.bmin[j][d];
        }
        
        // Contents on negative side
        temp.copy(parent_bounds);
        temp.bmax[0][d] = my_bounds.bmax[0][d];
        temp_area = temp.surface_area();
        if(temp_area < area)
        {
            area = temp_area;
            plane_axis = d;
            plane_neg = true;
            for(int j=0; j < nodes[me].plane.state_count; j++)
                nodes[me].plane[j] = my_bounds.bmax[j][d];
        }
    }
    
    std::cout << plane_axis << " " << plane_neg << " " << nodes[me].plane[0] << std::endl;
    
    // Calculate this node's actual carved bounds
    my_bounds.copy(parent_bounds);
    if(plane_neg)
    {
        for(int i=0; i < my_bounds.bmax.state_count; i++)
            my_bounds.bmax[i][plane_axis] = nodes[me].plane[i];
    }
    else
    {
        for(int i=0; i < my_bounds.bmin.state_count; i++)
            my_bounds.bmin[i][plane_axis] = nodes[me].plane[i];
    }
    
    std::cout << "X: " << my_bounds.bmin[0].x << " " << my_bounds.bmax[0].x << std::endl;
    std::cout << "Y: " << my_bounds.bmin[0].y << " " << my_bounds.bmax[0].y << std::endl;
    std::cout << "Z: " << my_bounds.bmin[0].z << " " << my_bounds.bmax[0].z << std::endl;
    
    // Record the split axis and direction for the node
    switch(plane_axis)
    {
        case 0:
            nodes[me].flags |= X_BOUND;
            break;
        
        case 1:
            nodes[me].flags |= Y_BOUND;
            break;
        
        case 2:
            nodes[me].flags |= Z_BOUND;
            break;
    }
    if(plane_neg)
        nodes[me].flags |= NEG_BOUND;
    
    // Leaf node?
    if(first_prim == last_prim)
    {
        std::cout << "Leaf" << std::endl;
        std::cout << "X: " << my_bounds.bmin[0].x << " " << my_bounds.bmax[0].x << std::endl;
        std::cout << "Y: " << my_bounds.bmin[0].y << " " << my_bounds.bmax[0].y << std::endl;
        std::cout << "Z: " << my_bounds.bmin[0].z << " " << my_bounds.bmax[0].z << std::endl;

        nodes[me].flags |= IS_LEAF;
        nodes[me].data = bag[first_prim].data;
        return;
    }

    // Not a leaf
    unsigned int child1i = next_node;
    unsigned int child2i = next_node + 1;
    next_node += 2;
    nodes[me].child_index = child1i;
    
    // Create child nodes
    int axis;
    unsigned int split_index = split_primitives(first_prim, last_prim, &axis);
    switch(axis)
    {
        case 0:
            nodes[me].flags |= X_SPLIT;
            break;
            
        case 1:
            nodes[me].flags |= Y_SPLIT;
            break;
            
        case 2:
            nodes[me].flags |= Z_SPLIT;
            break;
        
        default:
            nodes[me].flags |= X_SPLIT;
            break;
    }    
    
    recursive_build(child1i, first_prim, split_index, my_bounds);
    recursive_build(child2i, split_index+1, last_prim, my_bounds);
}


BBox &SSH::bounds()
{
    return bbox;
}


bool SSH::intersect_ray(Ray &ray, Intersection *intersection)
{
    bool hit = false;
    bool slab_hit;
    std::vector<Primitive *> temp_prim;
    BBox temp_bb;

    unsigned int todo[64];
    float todo_t_near[64];
    float todo_t_far[64];
    unsigned int todo_offset, node;
    
    float t, t_hit, t_near, t_far;
    unsigned int axis;
    
    t_hit = ray.maxt;
    
    //std::cout << ray.d[0]  << " " << ray.d[1] << " " << ray.d[2] << std::endl;
    
    //std::cout << "Yar1" << std::endl;
    // Check against SSH bounds
    if(!fast_intersect_test_bbox(bbox, ray, t_near, t_far))
    {
        return false;
    }
    
    // Skip the first node, because it's useless
    todo_offset = 1;
    node = 1;
    todo[0] = 2;
    todo_t_near[0] = t_near;
    todo_t_far[0] = t_far;
    
    // Traverse the SSH and check for intersections. Yay!
    while(true)
    {
        //std::cout << "Node: " << node << std::endl;
        
        // Test for intersection
        axis = nodes[node].flags & BOUND_MASK;
        slab_hit = false;
        if(ray.d[axis] != 0.0)
        {
            //std::cout << "Yar3" << std::endl;
            t = (nodes[node].plane[0] - ray.o[axis]) * ray.accel.inv_d[axis];
            if(nodes[node].flags & NEG_BOUND)
            {
                if(ray.accel.d_is_neg[axis])
                    t_near = t > t_near ? t : t_near;
                else
                    t_far = t < t_far ? t : t_far;
            }
            else
            {
                if(ray.accel.d_is_neg[axis])
                    t_far = t < t_far ? t : t_far;
                else
                    t_near = t > t_near ? t : t_near;
            }
            
            slab_hit = !((t_near > t_far) || (t_near > t_hit));
        }
        
        // If the ray intersects the current bounds
        if(slab_hit)
        {
            if(nodes[node].flags & IS_LEAF)
            {
                if(nodes[node].data->is_traceable(ray.min_width(t_near, t_far)))
                {
                    // Trace!
                    if(nodes[node].data->intersect_ray(ray, intersection))
                    {
                        hit = true;
                        t_hit = intersection->d < t_hit ? intersection->d : t_hit;
                    }
                
                    if(todo_offset == 0)
                        break;
                        
                    node = todo[--todo_offset];
                    t_near = todo_t_near[todo_offset];
                    t_far = todo_t_far[todo_offset];
                }
                else
                {
                    // TODO: do this better, especially the bounding box passed
                    // Split!
                    std::cout << "Split! " << node << std::endl; std::cout.flush();
                    temp_bb.copy(nodes[node].data->bounds());
                    nodes[node].data->refine(temp_prim);
                    add_primitives(temp_prim);
                    temp_prim.resize(0);
                    delete nodes[node].data;
                    
                    recursive_build(node, 0, bag.size()-1, temp_bb);
                    bag.resize(0);
                }
            }
            else
            {
                // Put far SSH node on todo stack, advance to near node
                // Which child node is near and which is far?
                bool first=true;
                switch(nodes[node].flags & SPLIT_MASK)
                {
                    case X_SPLIT:
                        if(ray.d.x < 0)
                            first = false;
                        break;
                        
                    case Y_SPLIT:
                        if(ray.d.y < 0)
                            first = false;
                        break;
                        
                    case Z_SPLIT:
                        if(ray.d.z < 0)
                            first = false;
                        break;
                    
                    default:
                        first = true;
                }
                
                // Put far SSH node on todo stack, advance to near node
                todo_t_near[todo_offset] = t_near;
                todo_t_far[todo_offset] = t_far;
                if(first)
                {
                    todo[todo_offset++] = nodes[node].child_index + 1;
                    node = nodes[node].child_index;
                }
                else
                {
                    todo[todo_offset++] = nodes[node].child_index;
                    node = nodes[node].child_index + 1;
                }
            }
        }
        else
        {
            if(todo_offset == 0)
                break;
            
            node = todo[--todo_offset];
            t_near = todo_t_near[todo_offset];
            t_far = todo_t_far[todo_offset];
        }
    }
    
    return hit;
}
