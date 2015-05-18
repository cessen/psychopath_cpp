#ifndef MATRIX_HPP
#define MATRIX_HPP

#include "numtype.h"

#include "vector.hpp"
#include "ImathMatrix.h"

#include <cstring>




#if 0

struct alignas(16) Matrix44 {
    float data[4][4];


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
	for (int i = 0; i < 4; ++i) {
		r[i][0] = data[i][0] * n;
		r[i][1] = data[i][1] * n;
		r[i][2] = data[i][2] * n;
		r[i][3] = data[i][3] * n;
	}
	return r;
}
Matrix44& operator*=(float n) {
	for (int i = 0; i < 4; ++i) {
		data[i][0] *= n;
		data[i][1] *= n;
		data[i][2] *= n;
		data[i][3] *= n;
	}
	return *this;
}
Matrix44 operator/(float n) const {
	Matrix44 r;
	for (int i = 0; i < 4; ++i) {
		r[i][0] = data[i][0] / n;
		r[i][1] = data[i][1] / n;
		r[i][2] = data[i][2] / n;
		r[i][3] = data[i][3] / n;
	}
	return r;
}
Matrix44& operator/=(float n) {
	for (int i = 0; i < 4; ++i) {
		data[i][0] /= n;
		data[i][1] /= n;
		data[i][2] /= n;
		data[i][3] /= n;
	}
	return *this;
}


// Matrix/Vector operations
void multVecMatrix(const Vec3 &src, Vec3 &dst) const {
	float w;

	w = src[0] * data[0][3] + src[1] * data[1][3] + src[2] * data[2][3] + data[3][3];
	dst.x = (src[0] * data[0][0] + src[1] * data[1][0] + src[2] * data[2][0] + data[3][0]) / w;
	dst.y = (src[0] * data[0][1] + src[1] * data[1][1] + src[2] * data[2][1] + data[3][1]) / w;
	dst.z = (src[0] * data[0][2] + src[1] * data[1][2] + src[2] * data[2][2] + data[3][2]) / w;
}

void multDirMatrix(const Vec3 &src, Vec3 &dst) const {
	dst.x = src[0] * data[0][0] + src[1] * data[1][0] + src[2] * data[2][0];
	dst.y = src[0] * data[0][1] + src[1] * data[1][1] + src[2] * data[2][1];
	dst.z = src[0] * data[0][2] + src[1] * data[1][2] + src[2] * data[2][2];
}


// Matrix/Matrix operations
Matrix44 operator+(const Matrix44& m) const {
	Matrix44 r;
	for (int i = 0; i < 4; ++i) {
		r[i][0] = data[i][0] + m[i][0];
		r[i][1] = data[i][1] + m[i][1];
		r[i][2] = data[i][2] + m[i][2];
		r[i][3] = data[i][3] + m[i][3];
	}
	return r;
}
Matrix44& operator+=(const Matrix44& m) {
	for (int i = 0; i < 4; ++i) {
		data[i][0] += m[i][0];
		data[i][1] += m[i][1];
		data[i][2] += m[i][2];
		data[i][3] += m[i][3];
	}
	return *this;
}
Matrix44 operator-(const Matrix44& m) const {
	Matrix44 r;
	for (int i = 0; i < 4; ++i) {
		r[i][0] = data[i][0] - m[i][0];
		r[i][1] = data[i][1] - m[i][1];
		r[i][2] = data[i][2] - m[i][2];
		r[i][3] = data[i][3] - m[i][3];
	}
	return r;
}
Matrix44& operator-=(const Matrix44& m) {
	for (int i = 0; i < 4; ++i) {
		data[i][0] -= m[i][0];
		data[i][1] -= m[i][1];
		data[i][2] -= m[i][2];
		data[i][3] -= m[i][3];
	}
	return *this;
}
Matrix44 operator*(const Matrix44& m) const {
	Matrix44 r;

	r[0][0] = data[0][0] * m[0][0] + data[0][1] * m[1][0] + data[0][2] * m[2][0] + data[0][3] * m[3][0];
	r[0][1] = data[0][0] * m[0][1] + data[0][1] * m[1][1] + data[0][2] * m[2][1] + data[0][3] * m[3][1];
	r[0][2] = data[0][0] * m[0][2] + data[0][1] * m[1][2] + data[0][2] * m[2][2] + data[0][3] * m[3][2];
	r[0][3] = data[0][0] * m[0][3] + data[0][1] * m[1][3] + data[0][2] * m[2][3] + data[0][3] * m[3][3];

	r[1][0] = data[1][0] * m[0][0] + data[1][1] * m[1][0] + data[1][2] * m[2][0] + data[1][3] * m[3][0];
	r[1][1] = data[1][0] * m[0][1] + data[1][1] * m[1][1] + data[1][2] * m[2][1] + data[1][3] * m[3][1];
	r[1][2] = data[1][0] * m[0][2] + data[1][1] * m[1][2] + data[1][2] * m[2][2] + data[1][3] * m[3][2];
	r[1][3] = data[1][0] * m[0][3] + data[1][1] * m[1][3] + data[1][2] * m[2][3] + data[1][3] * m[3][3];

	r[2][0] = data[2][0] * m[0][0] + data[2][1] * m[1][0] + data[2][2] * m[2][0] + data[2][3] * m[3][0];
	r[2][1] = data[2][0] * m[0][1] + data[2][1] * m[1][1] + data[2][2] * m[2][1] + data[2][3] * m[3][1];
	r[2][2] = data[2][0] * m[0][2] + data[2][1] * m[1][2] + data[2][2] * m[2][2] + data[2][3] * m[3][2];
	r[2][3] = data[2][0] * m[0][3] + data[2][1] * m[1][3] + data[2][2] * m[2][3] + data[2][3] * m[3][3];

	r[3][0] = data[3][0] * m[0][0] + data[3][1] * m[1][0] + data[3][2] * m[2][0] + data[3][3] * m[3][0];
	r[3][1] = data[3][0] * m[0][1] + data[3][1] * m[1][1] + data[3][2] * m[2][1] + data[3][3] * m[3][1];
	r[3][2] = data[3][0] * m[0][2] + data[3][1] * m[1][2] + data[3][2] * m[2][2] + data[3][3] * m[3][2];
	r[3][3] = data[3][0] * m[0][3] + data[3][1] * m[1][3] + data[3][2] * m[2][3] + data[3][3] * m[3][3];

	return r;
}
Matrix44& operator*=(const Matrix44& m) {
	*this = *this * m;

	return *this;
}


// Inversion
Matrix44 inverse() const {
	// Code pulled from ImathMatrix.h, part of the IlmBase library.
	int i, j, k;
	Matrix44 s;
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
			// Cannot invert singular matrix
			assert(false);
			return Matrix44();
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
			// Cannot invert singular matrix
			assert(false);
			return Matrix44();
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
