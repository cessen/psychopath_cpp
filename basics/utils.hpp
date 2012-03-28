#ifndef UTILS_HPP
#define UTILS_HPP

#include <math.h>

/*
   linear interpolation
   alpha = 0.0 means full a
   alpha = 1.0 means full b
 */

template <class T>
static inline T lerp(const float &alpha, const T &a, const T &b)
{
    return (a * (1.0-alpha)) + (b * alpha);
}


template <class T>
static inline T lerp2d(float alpha_u, float alpha_v,
                       T s00, T s10, T s01, T s11)
{
    T temp1 = (s00 * (1.0-alpha_u)) + (s10 * alpha_u);
    T temp2 = (s01 * (1.0-alpha_u)) + (s11 * alpha_u);
    return (temp1 * (1.0-alpha_v)) + (temp2 * alpha_v);
}

#define QPI (3.1415926535897932384626433 / 4)

/*
 * Maps the unit square to the unit circle.
 * Modifies x and y in place.
 */
static inline void square_to_circle(float *x, float *y)
{
    //std::cout << "In: " << *x << " " << *y << std::endl;
    
    if(*x == 0 && *y == 0)
        return;
        
    float radius, angle;
    
    if(*x > fabs(*y)) // Quadrant 1
    {
        radius = *x;
        angle = QPI * (*y/ *x);
    }
    else if (*y > fabs(*x)) // Quadrant 2
    {
        radius = *y;
        angle = QPI * (2 - (*x/ *y));
    }
    else if(*x < -fabs(*y)) // Quadrant 3
    {
        radius = -*x;
        angle = QPI * (4 + (*y/ *x));
    }
    else // Quadrant 4
    {
        radius = -*y;
        angle = QPI * (6 - (*x/ *y));
    }
    
    *x = radius * cos(angle);
    *y = radius * sin(angle);
    
    //std::cout << "Out: " << *x << " " << *y << std::endl;
}


/*
 * Quick lookup of what indices and alpha we should use to interpolate
 * time samples.
 * The first index and alpha are put into i and alpha respectively.
 */
static inline bool calc_time_interp(const unsigned char& time_count, const float &time, unsigned int *i, float *alpha)
{
    if(time_count < 2)
        return false;
    
    if(time < 1.0)
    {
        const float temp = time * (time_count - 1);
        *i = temp;
        *alpha = temp - (float)(*i);
    }
    else
    {
        *i = time_count - 2;
        *alpha = 1.0;
    }
    
    return true;
}


#endif
