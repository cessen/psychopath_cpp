#ifndef VECTOR_HPP
#define VECTOR_HPP

#include "numtype.h"

#include "OSL/Imathx.h"

// 3D vector
typedef Imath::Vec3<float32> Vec3;

template <class T>
static inline float32 dot(const T &a, const T &b)
{
	return a.dot(b);
}

template <class T>
static inline T cross(const T &a, const T &b)
{
	return a.cross(b);
}

#endif
