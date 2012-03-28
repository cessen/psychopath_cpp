#include "bbox.hpp"
#include "utils.hpp"

BBox::BBox(const int &res_time)
{
    bmin.init(res_time);
    bmax.init(res_time);
}


BBox::BBox(const Vec3 &bmin_, const Vec3 &bmax_)
{
    bmin.init(1);
    bmax.init(1);
    bmin[0] = bmin_;
    bmax[0] = bmax_;
}


void BBox::add_time_sample(const int &samp, const Vec3 &bmin_, const Vec3 &bmax_)
{
    bmin[samp] = bmin_;
    bmax[samp] = bmax_;
}


bool BBox::intersect_ray_(Ray &ray, float *hitt0, float *hitt1)
{
    float t0 = ray.mint;
    float t1 = ray.maxt;
    int ia=0, ib=0;
    float alpha=0.0;
    float bminf, bmaxf; // Store time-interpolated bbox values
    bool motion;
    
    motion = bmin.query_time(ray.time, &ia, &ib, &alpha);
    
    for (int i = 0; i < 3; ++i) {
        if(motion)
        {
            bminf = lerp(alpha, bmin[ia][i], bmin[ib][i]);
            bmaxf = lerp(alpha, bmax[ia][i], bmax[ib][i]);
        }
        else
        {
            bminf = bmin[0][i];
            bmaxf = bmax[0][i];
        }
        
        float inv_ray_dir = 1.f / ray.d[i];
        float t_near = (bminf - ray.o[i]) * inv_ray_dir;
        float t_far = (bmaxf - ray.o[i]) * inv_ray_dir;
        
        if(t_near > t_far)
        {
            float temp = t_near;
            t_near = t_far;
            t_far = temp;
        }
        
        t0 = t_near > t0 ? t_near : t0;
        t1 = t_far < t1 ? t_far : t1;
        if(t0 > t1)
            return false;
    }
    
    if (hitt0)
        *hitt0 = t0;
    if (hitt1)
        *hitt1 = t1;

    return true;
}


void BBox::copy(const BBox &b)
{
    if(bmin.state_count != b.bmin.state_count)
    {
        bmin.init(b.bmin.state_count);
        bmax.init(b.bmin.state_count);
    }
    
    for(int time=0; time < bmin.state_count; time++)
    {
        for(int i=0; i < 3; i++)
        {
            bmin[time][i] = b.bmin[time][i];
            bmax[time][i] = b.bmax[time][i];
        }
    }
}


void BBox::merge(const BBox &b)
{
    if(bmin.state_count == b.bmin.state_count)
    {
        // BBoxes have same state count
        for(int time=0; time < bmin.state_count; time++)
        {
            for(int i=0; i < 3; i++)
            {
                bmin[time][i] = bmin[time][i] < b.bmin[time][i] ? bmin[time][i] : b.bmin[time][i];
                bmax[time][i] = bmax[time][i] > b.bmax[time][i] ? bmax[time][i] : b.bmax[time][i];
            }
        }
    }
    else
    {
        // BBoxes have different state count
        // Merge them both into single-state bbox
        
        BBox bb;
        bb.bmin[0] = bmin[0];
        bb.bmax[0] = bmax[0];
        
        // First bbox
        for(int time=1; time < bmin.state_count; time++)
        {
            for(int i=0; i < 3; i++)
            {
                bb.bmin[0][i] = bmin[time][i] < bb.bmin[0][i] ? bmin[time][i] : bb.bmin[0][i];
                bb.bmax[0][i] = bmax[time][i] > bb.bmax[0][i] ? bmax[time][i] : bb.bmax[0][i];
            }
        }
        
        // Second bbox
        for(int time=0; time < b.bmin.state_count; time++)
        {
            for(int i=0; i < 3; i++)
            {
                bb.bmin[0][i] = b.bmin[time][i] < bb.bmin[0][i] ? b.bmin[time][i] : bb.bmin[0][i];
                bb.bmax[0][i] = b.bmax[time][i] > bb.bmax[0][i] ? b.bmax[time][i] : bb.bmax[0][i];
            }
        }
        
        copy(bb);
    }
}

