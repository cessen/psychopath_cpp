#ifndef VECTOR_H
#define VECTOR_H

#include "numtype.h"

#include <iostream>
#include <stdlib.h>
#include <math.h>

struct Vec3 {
	float32 x, y, z;

	Vec3(const float32 &x_=0.0, const float32 &y_=0.0, const float32 &z_=0.0) {
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

	Vec3 operator*(const float32 &b) const {
		return Vec3(x*b, y*b, z*b);
	}

	Vec3 operator/(const float32 &b) const {
		return Vec3(x/b, y/b, z/b);
	}

	float32 length() const {
		return sqrt(x*x + y*y + z*z);
	}

	float32 normalize() {
		const float32 l = length();
		x /= l;
		y /= l;
		z /= l;
		return l;
	}

	float32 &operator[](const int &i) {
		return (&x)[i];
	}

	float32 const &operator[](const int &i) const {
		return (&x)[i];
	}
};

inline float32 dot(const Vec3 &v1, const Vec3 &v2)
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
