#ifndef UTILS_HPP
#define UTILS_HPP

#include "numtype.h"

#include "vector.hpp"

#include <iterator>
#include <cmath>
#include <assert.h>

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
	const T temp1 = (s00 * (1.0-alpha_u)) + (s10 * alpha_u);
	const T temp2 = (s01 * (1.0-alpha_u)) + (s11 * alpha_u);
	return (temp1 * (1.0-alpha_v)) + (temp2 * alpha_v);
}


/*
 * Quick lookup of what indices and alpha we should use to interpolate
 * time samples.
 * The first index and alpha are put into i and alpha respectively.
 */
static inline bool calc_time_interp(const uint8_t& time_count, const float &time, uint32_t *i, float *alpha)
{
	if (time_count < 2)
		return false;

	if (time < 1.0) {
		const float temp = time * (time_count - 1);
		*i = temp;
		*alpha = temp - (float)(*i);
	} else {
		*i = time_count - 2;
		*alpha = 1.0;
	}

	return true;
}

template <class T, class iterator>
static inline const T lerp_seq(const float &alpha, const iterator &seq, const size_t &seq_length)
{
	if (seq_length == 1)
		return seq[0];
	else if (seq_length == 2)
		return lerp(alpha, seq[0], seq[1]);
	else if (alpha < 1.0) {
		const float temp = alpha * (seq_length - 1);
		const size_t index = static_cast<size_t>(temp);
		const float nalpha = temp - index;
		return lerp(nalpha, seq[index], seq[index+1]);
	}

	return seq[seq_length-1];
}

#define QPI (3.1415926535897932384626433 / 4)

/*
 * Maps the unit square to the unit circle.
 * Modifies x and y in place.
 * NOTE: x and y should be distributed within [-1, 1],
 * not [0, 1].
 */
static inline void square_to_circle(float *x, float *y)
{
	assert(*x >= -1.0 && *x <= 1.0 && *y >= -1.0 && *y <= 1.0);

	if (*x == 0.0 && *y == 0.0)
		return;

	float radius, angle;

	if (*x > std::abs(*y)) { // Quadrant 1
		radius = *x;
		angle = QPI * (*y/ *x);
	} else if (*y > std::abs(*x)) { // Quadrant 2
		radius = *y;
		angle = QPI * (2 - (*x/ *y));
	} else if (*x < -std::abs(*y)) { // Quadrant 3
		radius = -*x;
		angle = QPI * (4 + (*y/ *x));
	} else { // Quadrant 4
		radius = -*y;
		angle = QPI * (6 - (*x/ *y));
	}

	*x = radius * std::cos(angle);
	*y = radius * std::sin(angle);
}

static inline Vec3 cosine_sample_hemisphere(float u, float v)
{
	u = (u*2)-1;
	v = (v*2)-1;
	square_to_circle(&u, &v);
	const float z = std::sqrt(std::max(0.0, 1.0 - ((u*u) + (v*v))));
	return Vec3(u, v, z);
}

static inline Vec3 cosine_sample_hemisphere_polar(float u, float v)
{
	const float r = std::sqrt(u);
	const float theta = 2 * M_PI * v;

	const float x = r * std::cos(theta);
	const float y = r * std::sin(theta);

	return Vec3(x, y, std::sqrt(std::max(0.0f, 1 - u)));
}

static inline Vec3 uniform_sample_hemisphere(float u, float v)
{
	float z = u;
	float r = std::sqrt(std::max(0.f, 1.f - z*z));
	float phi = 2 * M_PI * v;
	float x = r * std::cos(phi);
	float y = r * std::sin(phi);
	return Vec3(x, y, z);
}

static inline Vec3 uniform_sample_sphere(float u, float v)
{
	float z = 1.f - (2.f * u);
	float r = std::sqrt(std::max(0.f, 1.f - z*z));
	float phi = 2.f * M_PI * v;
	float x = r * std::cos(phi);
	float y = r * std::sin(phi);
	return Vec3(x, y, z);
}


/**
 * @brief Creates a coordinate system from a single vector.
 */
static inline void coordinate_system_from_vec3(const Vec3 &v1, Vec3 *v2, Vec3 *v3)
{
	if (std::abs(v1.x) > std::fabs(v1.y)) {
		float invlen = 1.f / std::sqrt(v1.x*v1.x + v1.z*v1.z);
		*v2 = Vec3(-v1.z * invlen, 0.f, v1.x * invlen);
	} else {
		float invlen = 1.f / std::sqrt(v1.y*v1.y + v1.z*v1.z);
		*v2 = Vec3(0.f, v1.z * invlen, -v1.y * invlen);
	}
	*v3 = cross(v1, *v2);
}


/**
 * Simple mapping of a vector that exists in a z-up space to
 * the space of another vector who's direction is considered
 * z-up for the purpose.
 * Obviously this doesn't care about the direction _around_
 * the z-up, although it will be sufficiently consistent for
 * isotropic sampling purposes.
 *
 * @param from The vector we're transforming.
 * @param toz The vector whose space we are transforming "from" into.
 *
 * @return The transformed vector.
 */
static inline Vec3 zup_to_vec(Vec3 from, Vec3 toz)
{
	Vec3 tox, toy;
	toz.normalize();
	coordinate_system_from_vec3(toz, &tox, &toy);

	// Use simple linear algebra to convert the "from"
	// vector to a space composed of tox, toy, and toz
	// as the x, y, and z axes.
	return (tox * from.x) + (toy * from.y) + (toz * from.z);
}


/**
 * @brief Returns the log base 2 of the given integer.
 */
static inline uint32_t intlog2(uint32_t v)
{
	uint32_t r;
	uint32_t shift;

	r = (v > 0xFFFF) << 4;
	v >>= r;
	shift = (v > 0xFF) << 3;
	v >>= shift;
	r |= shift;
	shift = (v > 0xF) << 2;
	v >>= shift;
	r |= shift;
	shift = (v > 0x3) << 1;
	v >>= shift;
	r |= shift;
	r |= (v >> 1);

	return r;
}

/**
 * @brief Rounds an integer up to the next power of two.
 */
static inline uint32_t upper_power_of_two(uint32_t v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;

}


/**
 * @brief Fast approximation of log2.
 *
 * Taken from http://fastapprox.googlecode.com/
 * (C) 2011 Paul Mineiro
 * Under a BSD style license.  See above URL for details.
 */
static inline float fastlog2(float x)
{
	union {
		float f;
		uint32_t i;
	} vx = { x };
	union {
		uint32_t i;
		float f;
	} mx = { (vx.i & 0x007FFFFF) | 0x3f000000 };
	float y = vx.i;
	y *= 1.1920928955078125e-7f;

	return y - 124.22551499f
	       - 1.498030302f * mx.f
	       - 1.72587999f / (0.3520887068f + mx.f);
}

/**
 * @brief An even faster (but less accurate) approximation of log2.
 *
 * Taken from http://fastapprox.googlecode.com/
 * (C) 2011 Paul Mineiro
 * Under a BSD style license.  See above URL for details.
 */
static inline float fasterlog2(float x)
{
	union {
		float f;
		uint32_t i;
	} vx = { x };
	float y = vx.i;
	y *= 1.1920928955078125e-7f;
	return y - 126.94269504f;
}


static inline float fastrsqrt(float n)
{
	union {
		int32_t i;
		float y;
	};
	float x;
	
	x = n * 0.5F;
	y = n;
	i = 0x5f3759df - ( i >> 1 );

	return y;
}

static inline std::string to_string(const __m128& v)
{
	float vs[4];
	_mm_store_ps(vs, v);

	std::string s;
	s.append("(");
	s.append(std::to_string(vs[0]));
	s.append(", ");
	s.append(std::to_string(vs[1]));
	s.append(", ");
	s.append(std::to_string(vs[2]));
	s.append(", ");
	s.append(std::to_string(vs[3]));
	s.append(")");
	return s;
}


#endif
