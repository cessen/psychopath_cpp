#ifndef MATRIX_HPP
#define MATRIX_HPP

#include "numtype.h"

#include "vector.hpp"
#include "ImathMatrix.h"

// 4x4 transform matrix
typedef Imath::Matrix44<float32> Matrix44;

// Tranforms a vector with the transpose of a matrix
static inline ImathVec3 vec_transform_transpose(const Matrix44 &m, const ImathVec3 &v)
{
	ImathVec3 r;
	float32 w;

	r.x = v[0] * m[0][0] + v[1] * m[0][1] + v[2] * m[0][2] + m[0][3];
	r.y = v[0] * m[1][0] + v[1] * m[1][1] + v[2] * m[1][2] + m[1][3];
	r.z = v[0] * m[2][0] + v[1] * m[2][1] + v[2] * m[2][2] + m[2][3];
	w   = v[0] * m[3][0] + v[1] * m[3][1] + v[2] * m[3][2] + m[3][3];

	r.x /= w;
	r.y /= w;
	r.z /= w;

	return r;
}

// Tranforms a vector, as a direction, with the transpose of a matrix
static inline ImathVec3 dir_transform_transpose(const Matrix44 &m, const ImathVec3 &v)
{
	ImathVec3 r;

	r.x = v[0] * m[0][0] + v[1] * m[0][1] + v[2] * m[0][2];
	r.y = v[0] * m[1][0] + v[1] * m[1][1] + v[2] * m[1][2];
	r.z = v[0] * m[2][0] + v[1] * m[2][1] + v[2] * m[2][2];

	return r;
}

#endif
