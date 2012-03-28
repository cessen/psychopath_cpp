#include <iostream>
#include <stdlib.h>
#include "bilinear.hpp"
#include "grid.hpp"
#include "config.h"

Bilinear::Bilinear(unsigned int res_time_)
{
    last_rayw = 99999999999999999999999.0;
    has_bounds = false;
    verts.init(res_time_);
    
    for(unsigned int i=0; i < res_time_; i++)
    {
        verts[i] = new Vec3[4];
    }
       
    grid_key = 0;
}

Bilinear::Bilinear(Vec3 v1, Vec3 v2, Vec3 v3, Vec3 v4)
{
    last_rayw = 99999999999999999999999.0;
    has_bounds = false;
    verts.init(1);
    verts[0] = new Vec3[4];

    verts[0][0] = v1;
    verts[0][1] = v2;
    verts[0][2] = v3;
    verts[0][3] = v4;
    
    grid_key = 0;
}

Bilinear::~Bilinear()
{
    for(int i=0; i < verts.state_count; i++)
    {
        delete [] verts[i];
    }
}


void Bilinear::add_time_sample(int samp, Vec3 v1, Vec3 v2, Vec3 v3, Vec3 v4)
{
    verts[samp][0] = v1;
    verts[samp][1] = v2;
    verts[samp][2] = v3;
    verts[samp][3] = v4;
}


//////////////////////////////////////////////////////////////

int Bilinear::dice_rate(float upoly_width)
{   
    // Approximate size of the patch
    float size = (bounds().bmax[0] - bounds().bmin[0]).length() / 1.4;
        
    // Dicing rate based on ray width
    int rate = 1 + (size / (upoly_width * Config::dice_rate));
    if(rate < 2)
        rate = 2;
    
    return rate;
}


bool Bilinear::intersect_ray(Ray &ray, Intersection *intersection)
{
    // Dice the grid if we don't have one already
    if(!GridCache::cache.exists(grid_key))
    {
        // Get closest intersection with the bounding box
        float tnear, tfar;
        if(!bounds().intersect_ray_(ray, &tnear, &tfar))
            return false;
        
        // Get dicing rate
        int rate = dice_rate(ray.min_width(tnear, tfar));
        //std::cout << "Rate: " << rate << std::endl;
        
        // Dice away!
        dice(rate, rate);
    }
    
    GridCache::cache.touch(grid_key);
    return GridCache::cache[grid_key]->intersect_ray(ray, intersection);
}


BBox &Bilinear::bounds()
{
    if(!has_bounds)
    {
        bbox.bmin.init(verts.state_count);
        bbox.bmax.init(verts.state_count);
        
        for(int time = 0; time < verts.state_count; time++)
        {
            bbox.bmin[time].x = verts[time][0].x;
            bbox.bmax[time].x = verts[time][0].x;
            bbox.bmin[time].y = verts[time][0].y;
            bbox.bmax[time].y = verts[time][0].y;
            bbox.bmin[time].z = verts[time][0].z;
            bbox.bmax[time].z = verts[time][0].z;
            
            for(int i = 1; i < 4; i++)
            {
                bbox.bmin[time].x = verts[time][i].x < bbox.bmin[time].x ? verts[time][i].x : bbox.bmin[time].x;
                bbox.bmax[time].x = verts[time][i].x > bbox.bmax[time].x ? verts[time][i].x : bbox.bmax[time].x;
                bbox.bmin[time].y = verts[time][i].y < bbox.bmin[time].y ? verts[time][i].y : bbox.bmin[time].y;
                bbox.bmax[time].y = verts[time][i].y > bbox.bmax[time].y ? verts[time][i].y : bbox.bmax[time].y;
                bbox.bmin[time].z = verts[time][i].z < bbox.bmin[time].z ? verts[time][i].z : bbox.bmin[time].z;
                bbox.bmax[time].z = verts[time][i].z > bbox.bmax[time].z ? verts[time][i].z : bbox.bmax[time].z;
            }
        }
        has_bounds = true;
    }
    
    return bbox;
}


bool Bilinear::is_traceable(float ray_width)
{
    if(ray_width < last_rayw)
    {
        float lu = (verts[0][0] - verts[0][1]).length() + (verts[0][3] - verts[0][2]).length();
        float lv = (verts[0][0] - verts[0][3]).length() + (verts[0][1] - verts[0][2]).length();
        float edge_ratio = lu / lv;
        
        int r = dice_rate(ray_width);
        if(r <= Config::max_grid_size && (edge_ratio <= 1.5 && edge_ratio >= 0.75))
        {
            last_rayw = ray_width;
            return true;
        }
        else
        {
            return false;
        }
    }
    else
        return true;
}


void Bilinear::refine(std::vector<Primitive *> &primitives)
{
    Config::split_count++;
    
    primitives.resize(2);
    primitives[0] = new Bilinear(verts.state_count);
    primitives[1] = new Bilinear(verts.state_count);
    
    float lu;
    float lv;
    
    lu = (verts[0][0] - verts[0][1]).length() + (verts[0][3] - verts[0][2]).length();
    lv = (verts[0][0] - verts[0][3]).length() + (verts[0][1] - verts[0][2]).length();
    
    // TODO
    if(lu > lv)
    {
        //std::cout << "U\n";
        // Split on U
        for(int i=0; i < verts.state_count; i++)
        {
            ((Bilinear *)(primitives[0]))->add_time_sample(i,
                                              verts[i][0],
                                              (verts[i][0] + verts[i][1])*0.5,
                                              (verts[i][2] + verts[i][3])*0.5,
                                              verts[i][3]
                                              );
            ((Bilinear *)(primitives[1]))->add_time_sample(i,
                                              (verts[i][0] + verts[i][1])*0.5,
                                              verts[i][1],
                                              verts[i][2],
                                              (verts[i][2] + verts[i][3])*0.5
                                              );
        }
    }
    else
    {
        //std::cout << "V\n";
        // Split on V
        for(int i=0; i < verts.state_count; i++)
        {
            ((Bilinear *)(primitives[0]))->add_time_sample(i,
                                              verts[i][0],
                                              verts[i][1],
                                              (verts[i][1] + verts[i][2])*0.5,
                                              (verts[i][3] + verts[i][0])*0.5
                                              );
            ((Bilinear *)(primitives[1]))->add_time_sample(i,
                                              (verts[i][3] + verts[i][0])*0.5,
                                              (verts[i][1] + verts[i][2])*0.5,
                                              verts[i][2],
                                              verts[i][3]
                                              );
        }
    }
}

/*
 * Dice the patch into a micropoly grid.
 * ru and rv are the resolution of the grid in vertices
 * in the u and v directions.
 */
void Bilinear::dice(const int ru, const int rv)
{
    Config::upoly_gen_count += (ru-1)*(rv-1);
    Grid *grid = new Grid(ru, rv, verts.state_count, 0);
    Vec3 du1;
    Vec3 du2;
    Vec3 dv;
    Vec3 p1, p2, p3;
    int x, y;
    int i, time;
    
    // Dice for each time sample
    for(time = 0; time < verts.state_count; time++)
    {   
        // Deltas
        du1.x = (verts[time][1].x - verts[time][0].x) / (ru-1);
        du1.y = (verts[time][1].y - verts[time][0].y) / (ru-1);
        du1.z = (verts[time][1].z - verts[time][0].z) / (ru-1);
        
        du2.x = (verts[time][2].x - verts[time][3].x) / (ru-1);
        du2.y = (verts[time][2].y - verts[time][3].y) / (ru-1);
        du2.z = (verts[time][2].z - verts[time][3].z) / (ru-1);
        
        // Starting points
        p1 = verts[time][0];
        
        p2 = verts[time][3];
        
        // Walk along u
        for(x=0; x < ru; x++)
        {
            // Deltas
            dv = (p2 - p1) / (rv-1);
            
            // Starting point
            p3 = p1;
          
            // Walk along v
            for(y=0; y < rv; y++)
            { 
                // Set grid vertex coordinates
                i = ru*y+x;
                grid->verts[time][i].p = p3;  
                
                // Set variables
                if(time == 0 and grid->vars)
                    grid->vars[i] = 0.5;
                
                // Update point
                p3 = p3 + dv;
            }

            // Update points
            p1 = p1 + du1;
            p2 = p2 + du2;
        }
    }
    
    grid->calc_normals();
    grid->finalize();
    
    grid_key = GridCache::cache.add(grid);
}
