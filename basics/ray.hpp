#ifndef RAY_HPP
#define RAY_HPP

#include "vector.h"
#include "matrix.h"
#include "config.h"
#include <algorithm>
#include <math.h>

/*
 * Various data useful for fast ray intersection
 * tests with various things.
 */
struct RayAccel
{
    Vec3 inv_d;
    unsigned int d_is_neg[3];
    
    void init(Vec3 &o, Vec3 &d)
    {
        inv_d.x = 1.0 / d.x;
        inv_d.y = 1.0 / d.y;
        inv_d.z = 1.0 / d.z;
        
        d_is_neg[0] = inv_d.x < 0;
        d_is_neg[1] = inv_d.y < 0;
        d_is_neg[2] = inv_d.z < 0;
    }
    
};

/*
 * A ray in 3d space.
 * Includes information about ray differentials.
 */
struct Ray {
    Vec3 o, d; // Ray origin and direction
    float time; // Time coordinate
    float rdw; // Ray differential width at origin
    float rdd; // Ray differential delta
    float sw;  // Ray scattering width at origin
    float sd;  // Ray scattering delta
    float mint;
    float maxt;

    RayAccel accel;
    
    Ray(const Vec3 &o_=Vec3(0,0,0), const Vec3 &d_=Vec3(0,0,0),
        const float &time_ = 0.0,
        const float &rdw_ = 0.0,
        const float &rdd_ = 0.0)
    {
        o = o_;
        d = d_;
        
        time = time_;
        
        rdw = rdw_;
        rdd = rdd_;
        
        mint = 0.0;
        maxt = 99999999999999999999999999.0;
    }
    
    void finalize()
    {
        accel.init(o, d);
    }
    
    /*
     * Returns the ray width at a given distance or
     * distance-range along the ray.
     */
    float width(const float &t) const
    {
        const float size = fabs(rdw + (rdd*t));
        const float scatter = fabs(sw + (sd*t));

        return std::max(std::max(size, scatter), Config::min_upoly_size);
    }
    
    /*
     * Returns the minimum ray width within a distance range.
     */
    float min_width(const float &tnear, const float &tfar) const
    {
        float size_near = rdw + (rdd*tnear);
        const float size_far = rdw + (rdd*tfar);
        float scatter_near = sw + (sd*tnear);
        const float scatter_far = sw + (sd*tfar);

        // Are near and far sizes opposite signs, indicating
        // a cross over zero?
        if(((size_near <= 0) && (size_far > 0))
           || ((size_near >= 0) && (size_far < 0)))
        {
            size_near = 0.0;
        }
        else
        {
            size_near = std::min(fabs(size_near), fabs(size_far));
        }
        
        // Are near and far scatters opposite signs, indicating
        // a cross over zero?
        if(((scatter_near <= 0) && (scatter_far > 0))
           || ((scatter_near >= 0) && (scatter_far < 0)))
        {
            scatter_near = 0.0;
        }
        else
        {
            scatter_near = std::min(fabs(scatter_near), fabs(scatter_far));
        }
        
        return std::max(std::max(size_near, scatter_near), Config::min_upoly_size);
    }
    
    /*
     * Applies a matrix transform.
     * finalize() needs to be manually called after this.
     */
    void apply_matrix(const Matrix44 &m)
    {
        o = m.mult_pos(o);
        d = m.mult_dir(d);
    }
    
    
    
};

#endif
