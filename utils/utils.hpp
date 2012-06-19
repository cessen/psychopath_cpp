#ifndef UTILS_HPP
#define UTILS_HPP

#include "numtype.h"

#include "vector.hpp"

#include <math.h>

/*
   linear interpolation
   alpha = 0.0 means full a
   alpha = 1.0 means full b
 */

template <class T>
static inline T lerp(const float32 &alpha, const T &a, const T &b)
{
	return (a * (1.0-alpha)) + (b * alpha);
}


template <class T>
static inline T lerp2d(float32 alpha_u, float32 alpha_v,
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
 * NOTE: x and y should be distributed within [-1, 1],
 * not [0, 1].
 */
static inline void square_to_circle(float32 *x, float32 *y)
{
	assert(*x >= -1.0 && *x <= 1.0 && *y >= -1.0 && *y <= 1.0);

	if (*x == 0.0 && *y == 0.0)
		return;

	float32 radius, angle;

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

static inline Vec3 cosine_sample_hemisphere(float32 u, float32 v)
{
	u = (u*2)-1;
	v = (v*2)-1;
	square_to_circle(&u, &v);
	const float32 z = std::sqrt(std::max(0.0, 1.0 - ((u*u) + (v*v))));
	return Vec3(u, v, z);
}

static inline Vec3 cosine_sample_hemisphere_polar(float32 u, float32 v)
{
	const float32 r = std::sqrt(u);
	const float32 theta = 2 * M_PI * v;

	const float32 x = r * std::cos(theta);
	const float32 y = r * std::sin(theta);

	return Vec3(x, y, std::sqrt(std::max(0.0f, 1 - u)));
}

static inline Vec3 uniform_sample_hemisphere(float32 u, float32 v)
{
	float32 z = u;
	float32 r = std::sqrt(std::max(0.f, 1.f - z*z));
	float32 phi = 2 * M_PI * v;
	float32 x = r * std::cos(phi);
	float32 y = r * std::sin(phi);
	return Vec3(x, y, z);
}

static inline Vec3 uniform_sample_sphere(float32 u, float32 v)
{
	float32 z = 1.f - (2.f * u);
	float32 r = std::sqrt(std::max(0.f, 1.f - z*z));
	float32 phi = 2.f * M_PI * v;
	float32 x = r * std::cos(phi);
	float32 y = r * std::sin(phi);
	return Vec3(x, y, z);
}


/**
 * @brief Creates a coordinate system from a single vector.
 */
static inline void coordinate_system_from_vec3(const Vec3 &v1, Vec3 *v2, Vec3 *v3)
{
	if (std::abs(v1.x) > std::fabs(v1.y)) {
		float32 invlen = 1.f / std::sqrt(v1.x*v1.x + v1.z*v1.z);
		*v2 = Vec3(-v1.z * invlen, 0.f, v1.x * invlen);
	} else {
		float32 invlen = 1.f / std::sqrt(v1.y*v1.y + v1.z*v1.z);
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


/*
 * Quick lookup of what indices and alpha we should use to interpolate
 * time samples.
 * The first index and alpha are put into i and alpha respectively.
 */
static inline bool calc_time_interp(const uint8& time_count, const float32 &time, uint32 *i, float32 *alpha)
{
	if (time_count < 2)
		return false;

	if (time < 1.0) {
		const float32 temp = time * (time_count - 1);
		*i = temp;
		*alpha = temp - (float32)(*i);
	} else {
		*i = time_count - 2;
		*alpha = 1.0;
	}

	return true;
}


#endif
