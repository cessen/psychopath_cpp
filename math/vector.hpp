#ifndef VECTOR_HPP
#define VECTOR_HPP

#include "numtype.h"
#include <assert.h>
#include <x86intrin.h>
#include <cmath>

#include "ImathVec.h"

// 3D vector
typedef Imath::Vec3<float> ImathVec3;


#if 0
/**
 * @brief A 3d vector.
 *
 * Optionally accelerated by SSE instructions.
 */
struct __attribute__((aligned(16))) Vec3 {
    union {
        struct {
            float x,y,z,w;
        };
        __m128 m128;
    };

    // Constructors
Vec3() {}
Vec3(float x_, float y_, float z_, float w_=1.0f) {
	x = x_;
	y = y_;
	z = z_;
	w = w_;
}
Vec3(__m128 m) {
	m128 = m;
}

// Element access
float &operator[](uint_t n) {
	assert(n < 4);
	return (&x)[n];
}
const float &operator[](uint_t n) const {
	assert(n < 4);
	return (&x)[n];
}

// Comparisons
bool operator==(const Vec3 &b) const {
	return (x==b.x && y==b.y && z==b.z);
}

// Multiplication and division by scalar
Vec3 operator*(float b) const {
	return (Vec3)_mm_mul_ps(m128, _mm_set_ps(b,b,b,b));
}
Vec3 operator/(float b) const {
	return (Vec3)_mm_div_ps(m128, _mm_set_ps(b,b,b,b));
}

// Component-wise arithmetic
Vec3 operator+(const Vec3& b) const {
	return (Vec3)_mm_add_ps(m128, b.m128);
}
Vec3 operator-(const Vec3& b) const {
	return (Vec3)_mm_sub_ps(m128, b.m128);
}
Vec3 operator*(const Vec3& b) const {
	return (Vec3)_mm_mul_ps(m128, b.m128);
}
Vec3 operator/(const Vec3& b) const {
	return (Vec3)_mm_div_ps(m128, b.m128);
}

// Products
float dot(const Vec3 &b) const {
	return x*b.x + y*b.y + z*b.z;
}
Vec3 cross(const Vec3 &b) const {
	return (Vec3)_mm_sub_ps(
	           _mm_mul_ps(
	               _mm_shuffle_ps(m128, m128, _MM_SHUFFLE(3, 0, 2, 1)),
	               _mm_shuffle_ps(b.m128, b.m128, _MM_SHUFFLE(3, 1, 0, 2))),
	           _mm_mul_ps(
	               _mm_shuffle_ps(m128, m128, _MM_SHUFFLE(3, 1, 0, 2)),
	               _mm_shuffle_ps(b.m128, b.m128, _MM_SHUFFLE(3, 0, 2, 1)))
	       );
}

// Component-wise min and max
Vec3 min(const Vec3 &b) const {
	return (Vec3)_mm_min_ps(m128, b.m128);
}
Vec3 max(const Vec3 &b) const {
	return (Vec3)_mm_max_ps(m128, b.m128);
}

float length() const {
	Vec3 a = *this;
	a.w = 0.0f;

	__m128 &D = a.m128;
	D = _mm_mul_ps(D, D);
	D = _mm_hadd_ps(D, D);
	D = _mm_hadd_ps(D, D);

	D = _mm_sqrt_ps(D);

	return a.x;
}

float length2() const {
	Vec3 a = *this;
	a.w = 0.0f;

	__m128 &D = a.m128;
	D = _mm_mul_ps(D, D);
	D = _mm_hadd_ps(D, D);
	D = _mm_hadd_ps(D, D);

	return a.x;
}

const Vec3 &normalize() {
	w = 0.f;

	__m128 D = m128;
	D = _mm_mul_ps(D, D);
	D = _mm_hadd_ps(D, D);
	D = _mm_hadd_ps(D, D);

	// 1 iteration of Newton-raphson -- Idea from Intel's Embree.
	__m128 r = _mm_rsqrt_ps(D);
	r = _mm_add_ps(
	        _mm_mul_ps(_mm_set_ps(1.5f, 1.5f, 1.5f, 1.5f), r),
	        _mm_mul_ps(_mm_mul_ps(_mm_mul_ps(D, _mm_set_ps(-0.5f, -0.5f, -0.5f, -0.5f)), r), _mm_mul_ps(r, r)));

	m128 = _mm_mul_ps(m128, r);

	return *this;
}

};
#else
typedef Imath::Vec3<float> Vec3;
#endif


template <class T>
static inline float dot(const T &a, const T &b)
{
	return a.dot(b);
}

// Normalized dot product (i.e. the cosine of the angle between two vectors
template <class T>
static inline float dot_norm(const T& a, const T& b)
{
	const float length_product = a.length() * b.length();
	assert(length_product > 0.0f);
	return ((a.x * b.x) + (a.y * b.y) + (a.z * b.z)) / length_product;
}

template <class T>
static inline T cross(const T &a, const T &b)
{
	return a.cross(b);
}

static inline Vec3 min(const Vec3 &a, const Vec3 &b)
{
	Vec3 c;
	for (int i = 0; i < 3; i++)
		c[i] = a[i] < b[i] ? a[i] : b[i];
	return c;
}

static inline Vec3 max(const Vec3 &a, const Vec3 &b)
{
	Vec3 c;
	for (int i = 0; i < 3; i++)
		c[i] = a[i] > b[i] ? a[i] : b[i];
	return c;
}

static inline float longest_axis(const Vec3 &v)
{
	return std::max(std::max(std::abs(v.x), std::abs(v.y)), std::abs(v.z));
}

#endif // VECTOR_HPP
