#ifndef MATRIX_HPP
#define MATRIX_HPP

#include "numtype.h"

#include "vector.hpp"
#include "ImathMatrix.h"
#include "simd.hpp"

#include <limits>
#include <cassert>
#include <cstring>




#if 1

struct alignas(16) Matrix44 {
    union {
        float data[4][4];
        SIMD::float4 data_s[4];
    };


Matrix44() {}
Matrix44(const Matrix44& m) {
	std::memcpy((void*)data, (const void*)m.data, sizeof(float)*16);
}
Matrix44(float a, float b, float c, float d, float e, float f, float g, float h,
float i, float j, float k, float l, float m, float n, float o, float p) {
	data[0][0] = a;
	data[0][1] = b;
	data[0][2] = c;
	data[0][3] = d;
	data[1][0] = e;
	data[1][1] = f;
	data[1][2] = g;
	data[1][3] = h;
	data[2][0] = i;
	data[2][1] = j;
	data[2][2] = k;
	data[2][3] = l;
	data[3][0] = m;
	data[3][1] = n;
	data[3][2] = o;
	data[3][3] = p;
}

Matrix44& operator=(const Matrix44& m) {
	std::memcpy((void*)data, (const void*)m.data, sizeof(float)*16);
	return *this;
}


// Array-style access
float* operator[](int i) {
	return data[i];
}
const float* operator[](int i) const {
	return data[i];
}


// Matrix/Scalar operations
Matrix44 operator*(float n) const {
	Matrix44 r;
	r.data_s[0] = data_s[0] * n;
	r.data_s[1] = data_s[1] * n;
	r.data_s[2] = data_s[2] * n;
	r.data_s[3] = data_s[3] * n;
	return r;
}
Matrix44& operator*=(float n) {
	data_s[0] = data_s[0] * n;
	data_s[1] = data_s[1] * n;
	data_s[2] = data_s[2] * n;
	data_s[3] = data_s[3] * n;
	return *this;
}
Matrix44 operator/(float n) const {
	Matrix44 r;
	r.data_s[0] = data_s[0] / n;
	r.data_s[1] = data_s[1] / n;
	r.data_s[2] = data_s[2] / n;
	r.data_s[3] = data_s[3] / n;
	return r;
}
Matrix44& operator/=(float n) {
	data_s[0] = data_s[0] / n;
	data_s[1] = data_s[1] / n;
	data_s[2] = data_s[2] / n;
	data_s[3] = data_s[3] / n;
	return *this;
}


// Matrix/Vector operations
void multVecMatrix(const Vec3 &src, Vec3 &dst) const {
	SIMD::float4 r = (data_s[0] * src[0]) + (data_s[1] * src[1]) + (data_s[2] * src[2]) + data_s[3];
	r = r / r[3];

	dst.x = r[0];
	dst.y = r[1];
	dst.z = r[2];
}

void multDirMatrix(const Vec3 &src, Vec3 &dst) const {
	SIMD::float4 r = (data_s[0] * src[0]) + (data_s[1] * src[1]) + (data_s[2] * src[2]);

	dst.x = r[0];
	dst.y = r[1];
	dst.z = r[2];
}


// Matrix/Matrix operations
Matrix44 operator+(const Matrix44& m) const {
	Matrix44 r;
	r.data_s[0] = data_s[0] + m.data_s[0];
	r.data_s[1] = data_s[1] + m.data_s[1];
	r.data_s[2] = data_s[2] + m.data_s[2];
	r.data_s[3] = data_s[3] + m.data_s[3];
	return r;
}
Matrix44& operator+=(const Matrix44& m) {
	data_s[0] = data_s[0] + m.data_s[0];
	data_s[1] = data_s[1] + m.data_s[1];
	data_s[2] = data_s[2] + m.data_s[2];
	data_s[3] = data_s[3] + m.data_s[3];
	return *this;
}
Matrix44 operator-(const Matrix44& m) const {
	Matrix44 r;
	r.data_s[0] = data_s[0] - m.data_s[0];
	r.data_s[1] = data_s[1] - m.data_s[1];
	r.data_s[2] = data_s[2] - m.data_s[2];
	r.data_s[3] = data_s[3] - m.data_s[3];
	return r;
}
Matrix44& operator-=(const Matrix44& m) {
	data_s[0] = data_s[0] - m.data_s[0];
	data_s[1] = data_s[1] - m.data_s[1];
	data_s[2] = data_s[2] - m.data_s[2];
	data_s[3] = data_s[3] - m.data_s[3];
	return *this;
}
Matrix44 operator*(const Matrix44& m) const {
	Matrix44 r;

	r.data_s[0] = (m.data_s[0] * data[0][0]) + (m.data_s[1] * data[0][1]) + (m.data_s[2] * data[0][2]) + (m.data_s[3] * data[0][3]);
	r.data_s[1] = (m.data_s[0] * data[1][0]) + (m.data_s[1] * data[1][1]) + (m.data_s[2] * data[1][2]) + (m.data_s[3] * data[1][3]);
	r.data_s[2] = (m.data_s[0] * data[2][0]) + (m.data_s[1] * data[2][1]) + (m.data_s[2] * data[2][2]) + (m.data_s[3] * data[2][3]);
	r.data_s[3] = (m.data_s[0] * data[3][0]) + (m.data_s[1] * data[3][1]) + (m.data_s[2] * data[3][2]) + (m.data_s[3] * data[3][3]);

	return r;
}
Matrix44& operator*=(const Matrix44& m) {
	data_s[0] = (m.data_s[0] * data[0][0]) + (m.data_s[1] * data[0][1]) + (m.data_s[2] * data[0][2]) + (m.data_s[3] * data[0][3]);
	data_s[1] = (m.data_s[0] * data[1][0]) + (m.data_s[1] * data[1][1]) + (m.data_s[2] * data[1][2]) + (m.data_s[3] * data[1][3]);
	data_s[2] = (m.data_s[0] * data[2][0]) + (m.data_s[1] * data[2][1]) + (m.data_s[2] * data[2][2]) + (m.data_s[3] * data[2][3]);
	data_s[3] = (m.data_s[0] * data[3][0]) + (m.data_s[1] * data[3][1]) + (m.data_s[2] * data[3][2]) + (m.data_s[3] * data[3][3]);

	return *this;
}


// Inversion
Matrix44 gjInverse() const {
	// Code pulled from ImathMatrix.h, part of the IlmBase library.
	int i, j, k;
	Matrix44 s;
	s.makeIdentity();
	Matrix44 t(*this);

	// Forward elimination
	for (i = 0; i < 3 ; i++) {
		int pivot = i;

		float pivotsize = t[i][i];

		if (pivotsize < 0)
			pivotsize = -pivotsize;

		for (j = i + 1; j < 4; j++) {
			float tmp = t[j][i];

			if (tmp < 0)
				tmp = -tmp;

			if (tmp > pivotsize) {
				pivot = j;
				pivotsize = tmp;
			}
		}

		if (pivotsize == 0) {
			// Cannot invert singular matrix, return an all-NaN matrix.
			s.makeNan();
			return s;
		}

		if (pivot != i) {
			for (j = 0; j < 4; j++) {
				float tmp;

				tmp = t[i][j];
				t[i][j] = t[pivot][j];
				t[pivot][j] = tmp;

				tmp = s[i][j];
				s[i][j] = s[pivot][j];
				s[pivot][j] = tmp;
			}
		}

		for (j = i + 1; j < 4; j++) {
			float f = t[j][i] / t[i][i];

			for (k = 0; k < 4; k++) {
				t[j][k] -= f * t[i][k];
				s[j][k] -= f * s[i][k];
			}
		}
	}

	// Backward substitution
	for (i = 3; i >= 0; --i) {
		float f;

		if ((f = t[i][i]) == 0) {
			// Cannot invert singular matrix, return an all-NaN matrix.
			s.makeNan();
			return s;
		}

		for (j = 0; j < 4; j++) {
			t[i][j] /= f;
			s[i][j] /= f;
		}

		for (j = 0; j < i; j++) {
			f = t[j][i];

			for (k = 0; k < 4; k++) {
				t[j][k] -= f * t[i][k];
				s[j][k] -= f * s[i][k];
			}
		}
	}

	return s;
}

void gjInvert() {
	*this = gjInverse();
}

Matrix44 inverse() const {
	// Code pulled from ImathMatrix.h, part of the IlmBase library.
	if (data[0][3] != 0 || data[1][3] != 0 || data[2][3] != 0 || data[3][3] != 1)
		return gjInverse();

	Matrix44 s(data[1][1] * data[2][2] - data[2][1] * data[1][2],
	           data[2][1] * data[0][2] - data[0][1] * data[2][2],
	           data[0][1] * data[1][2] - data[1][1] * data[0][2],
	           0,

	           data[2][0] * data[1][2] - data[1][0] * data[2][2],
	           data[0][0] * data[2][2] - data[2][0] * data[0][2],
	           data[1][0] * data[0][2] - data[0][0] * data[1][2],
	           0,

	           data[1][0] * data[2][1] - data[2][0] * data[1][1],
	           data[2][0] * data[0][1] - data[0][0] * data[2][1],
	           data[0][0] * data[1][1] - data[1][0] * data[0][1],
	           0,

	           0,
	           0,
	           0,
	           1);

	float r = data[0][0] * s[0][0] + data[0][1] * s[1][0] + data[0][2] * s[2][0];

	if (std::abs(r) >= 1) {
		for (int i = 0; i < 3; ++i) {
			for (int j = 0; j < 3; ++j) {
				s[i][j] /= r;
			}
		}
	} else {
		float mr = std::abs(r) / std::numeric_limits<float>::min();

		for (int i = 0; i < 3; ++i) {
			for (int j = 0; j < 3; ++j) {
				if (mr > std::abs(s[i][j])) {
					s[i][j] /= r;
				} else {
					// Cannot invert singular matrix, return an all-NaN matrix.
					s.makeNan();
					return s;
				}
			}
		}
	}

	s[3][0] = -data[3][0] * s[0][0] - data[3][1] * s[1][0] - data[3][2] * s[2][0];
	s[3][1] = -data[3][0] * s[0][1] - data[3][1] * s[1][1] - data[3][2] * s[2][1];
	s[3][2] = -data[3][0] * s[0][2] - data[3][1] * s[1][2] - data[3][2] * s[2][2];

	return s;
}

void invert() {
	*this = inverse();
}


// Misc
const Matrix44& setAxisAngle(const Vec3& axis, float angle) {
	// Code pulled from ImathMatrix.h, part of the IlmBase library.
	Vec3 unit(axis.normalized());
	float sine   = std::sin(angle);
	float cosine = std::cos(angle);

	data[0][0] = unit[0] * unit[0] * (1 - cosine) + cosine;
	data[0][1] = unit[0] * unit[1] * (1 - cosine) + unit[2] * sine;
	data[0][2] = unit[0] * unit[2] * (1 - cosine) - unit[1] * sine;
	data[0][3] = 0;

	data[1][0] = unit[0] * unit[1] * (1 - cosine) - unit[2] * sine;
	data[1][1] = unit[1] * unit[1] * (1 - cosine) + cosine;
	data[1][2] = unit[1] * unit[2] * (1 - cosine) + unit[0] * sine;
	data[1][3] = 0;

	data[2][0] = unit[0] * unit[2] * (1 - cosine) + unit[1] * sine;
	data[2][1] = unit[1] * unit[2] * (1 - cosine) - unit[0] * sine;
	data[2][2] = unit[2] * unit[2] * (1 - cosine) + cosine;
	data[2][3] = 0;

	data[3][0] = 0;
	data[3][1] = 0;
	data[3][2] = 0;
	data[3][3] = 1;

	return *this;
}

void makeNan() {
	for (int i = 0; i < 4; ++i) {
		data[i][0] = std::numeric_limits<float>::quiet_NaN();
		data[i][1] = std::numeric_limits<float>::quiet_NaN();
		data[i][2] = std::numeric_limits<float>::quiet_NaN();
		data[i][3] = std::numeric_limits<float>::quiet_NaN();
	}
}

void makeIdentity() {
	std::memset(data, 0, sizeof(float)*16);
	data[0][0] = 1.0f;
	data[1][1] = 1.0f;
	data[2][2] = 1.0f;
	data[3][3] = 1.0f;
}

};

#else

// 4x4 transform matrix
typedef Imath::Matrix44<float> Matrix44;

#endif




// Tranforms a vector with the transpose of a matrix
static inline ImathVec3 vec_transform_transpose(const Matrix44 &m, const ImathVec3 &v)
{
	ImathVec3 r;
	float w;

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
