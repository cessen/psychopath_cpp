#include "numtype.h"

#include <iostream>
#include "ray.hpp"
#include "prim_array.h"



PrimArray::~PrimArray()
{
    int32 size = children.size();
    
    for(int32 i=0; i < size; i++)
    {
        delete children[i];
    }
}

void PrimArray::add_primitives(std::vector<Primitive *> &primitives)
{
    int32 start = children.size();
    int32 added = primitives.size();
    children.resize(start + added);
    
    for(int32 i=0; i < added; i++)
    {
        children[start + i] = primitives[i];
    }
}

bool PrimArray::finalize()
{
    return true;
}

BBox &PrimArray::bounds()
{
    return bbox;
}

bool PrimArray::intersect_ray(Ray &ray, Intersection *intersection)
{
    std::vector<Primitive *> temp_prim;
    float32 tnear, tfar;
    bool hit = false;
    int32 size = children.size();
    
    for(int32 i=0; i < size; i++)
    {
        if(children[i]->bounds().intersect_ray_(ray, &tnear, &tfar))
        {
            if(children[i]->is_traceable(ray.min_width(tnear, tfar)))
            {
                // Trace!
                if(children[i]->intersect_ray(ray, intersection))
                    hit = true;
            }
            else
            {
                std::cout << "Split! " << i << std::endl; std::cout.flush();
                // Split!
                children[i]->refine(temp_prim);
                delete children[i];
                children[i] = temp_prim[0];
                
                int32 first = size;
                size += (temp_prim.size() - 1);
                children.resize(size);
                
                for(int32 j=first; j < size; j++)
                {
                    children[j] = temp_prim[1+j-first];
                }
                i--;
            }
        }
    }
    
    return hit;
}
