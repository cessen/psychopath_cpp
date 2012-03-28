#ifndef SSH_H
#define SSH_H

/*
 * A "Single Slab Hierarchy" ray tracing acceleration structure.
 * See "Ray Tracing with the Single Slab Hierarchy"
 * by Eisemann et al. for more information.
 */

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
        
        SSHPrimitive()
        {
            data = NULL;
        }
        
        void init(Primitive *prim)
        {
            Vec bmin, bmax;
            
            // Store primitive pointer and get bounds
            data = prim;
            
            // Get bounds at time 0.5
            int ia, ib;
            float alpha;
            if(data->bounds().bmin.query_time(0.5, &ia, &ib, &alpha))
            {
                bmin = lerp(0.5, data->bounds().bmin[ia], data->bounds().bmin[ib]);
                bmax = lerp(0.5, data->bounds().bmax[ia], data->bounds().bmax[ib]);
            }
            else
            {
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
class SSHNode {
    public:
        TimeBox<float> plane;  // Potentially multiple planes for multiple time samples

        union {
            unsigned int child_index;
            Primitive *data;
        };
        
        unsigned char flags;
        
        SSHNode()
        {
            child_index = 0;
            data = NULL;
            flags = 0;
        }
};


/*
 * A collection of SSHNodes.  Like a resizable array.
 * Allocates nodes in chunks, for less RAM shuffling.
 */
class SSHNodes {
    public:
        std::vector<SSHNode *> nodes;
        unsigned int num_nodes;
        
        SSHNodes()
        {
            num_nodes = 0;
        }
        
        ~SSHNodes()
        {
            int s = nodes.size();
            for(int i=0; i < s; i++)
            {
                delete [] nodes[i];
            }
        }
        
        SSHNode &operator[](int i)
        {
            if(SSH_CHUNK_SIZE > 1)
                return (nodes[i/SSH_CHUNK_SIZE])[i%SSH_CHUNK_SIZE];
            else
                return *nodes[i];
        }
        
        unsigned int size()
        {
            return num_nodes;
        }
        
        void add_chunk()
        {
            int s = nodes.size();
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
        unsigned int next_node;
        std::vector<SSHPrimitive> bag;  // Temporary holding spot for primitives not yet added to the hierarchy
        
    public:
        SSH()
        {
            next_node = 0;
        }
        virtual ~SSH();
        
        // Inherited
        virtual void add_primitives(std::vector<Primitive *> &primitives);
        virtual bool finalize();
        
        virtual BBox &bounds();
        virtual bool intersect_ray(Ray &ray, Intersection *intersection=NULL);
        
        unsigned split_primitives(unsigned int first_prim, unsigned int last_prim, int *axis=NULL);
        void recursive_build(unsigned int me, unsigned int first_prim, unsigned int last_prim, BBox &parent_bounds);
};




#endif
