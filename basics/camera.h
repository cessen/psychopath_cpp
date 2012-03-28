#ifndef CAMERA_H
#define CAMERA_H

#include <cmath>
#include <vector>

#include "config.h"
#include "utils.hpp"
#include "timebox.hpp"
#include "vector.h"
#include "matrix.h"
#include "ray.hpp"

/*
 * A virtual camera.
 */
class Camera
{
    public:
        TimeBox<Matrix44> transforms;
        float fov, tfov;
        float lens_diameter, focus_distance;
        
        Camera(std::vector<Matrix44> &trans, float fov_, float lens_diameter_, float focus_distance_)
        {
            transforms.init(trans.size());
            for(unsigned int i=0; i < trans.size(); i++)
                transforms[i] = trans[i];
            
            fov = fov_;
            tfov = sin(fov/2) / cos(fov/2);
            
            lens_diameter = lens_diameter_;
            focus_distance = focus_distance_;
        }
        
        /*
         * Generates a camera ray based on the given information.
         */
        Ray generate_ray(float x, float y, float dx, float dy, float time, float u, float v) const
        {
            Ray ray;
            
            ray.time = time;
            
            // Ray origin
            ray.o.x = lens_diameter * ((u * 2) - 1) * 0.5;
            ray.o.y = lens_diameter * ((v * 2) - 1) * 0.5;
            ray.o.z = 0.0;
            square_to_circle(&ray.o.x, &ray.o.y);
            
            // Ray direction
            ray.d.x = (x * tfov) - (ray.o.x / focus_distance);
            ray.d.y = (y * tfov) - (ray.o.y / focus_distance);
            ray.d.z = 1.0;
            ray.d.normalize();
            
            // Ray differentials
            ray.rdw = 0.0;
            ray.rdd = (2.0 * dx * tfov) * ray.d.z;
            
            // Ray scattering differentials ("focus factor")
            ray.sw = 0.707 * lens_diameter * Config::focus_factor;
            ray.sd = (-ray.sw / focus_distance) * ray.d.z;
            
            // Get transform matrix
            unsigned int ia;
            float alpha;
            
            if(calc_time_interp(transforms.state_count, time, &ia, &alpha))
            {
                Matrix44 trans;
                trans = lerp(alpha, transforms[ia], transforms[ia+1]);
                
                ray.apply_matrix(trans);
            }
            else
            {
                ray.apply_matrix(transforms[0]);
            }
            
            return ray;
        }
};

#endif
