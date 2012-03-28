#ifndef VECTOR_H
#define VECTOR_H

#include <iostream>
#include <stdlib.h>
#include <math.h>

struct Vec3 {
    float x, y, z;
    
    Vec3(const float &x_=0.0, const float &y_=0.0, const float &z_=0.0)
    {
        x = x_;
        y = y_;
        z = z_;
    }
    
    Vec3 operator+(const Vec3 &b) const {
        return Vec3(x+b.x, y+b.y, z+b.z);
    }
    
    Vec3 operator-(const Vec3 &b) const {
        return Vec3(x-b.x, y-b.y, z-b.z);
    }
    
    Vec3 operator*(const float &b) const {
        return Vec3(x*b, y*b, z*b);
    }
    
    Vec3 operator/(const float &b) const {
        return Vec3(x/b, y/b, z/b);
    }
    
    float length() const {
        return sqrt(x*x + y*y + z*z);
    }
    
    float normalize() {
        const float l = length();
        x /= l;
        y /= l;
        z /= l;
        return l;
    }
    
    float &operator[](const int &i)
    {
        return (&x)[i];
    }
    
    float const &operator[](const int &i) const
    {
        return (&x)[i];
    }
};

inline float dot(const Vec3 &v1, const Vec3 &v2)
{
    return (v1.x * v2.x) + (v1.y * v2.y) + (v1.z * v2.z);
}

inline Vec3 cross(const Vec3 &v1, const Vec3 &v2)
{
    return Vec3((v1.y*v2.z)-(v1.z*v2.y),
                (v1.z*v2.x)-(v1.x*v2.z),
                (v1.x*v2.y)-(v1.y*v2.x));
}

#endif
