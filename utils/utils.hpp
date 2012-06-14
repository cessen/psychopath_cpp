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
 */
static inline void square_to_circle(float32 *x, float32 *y)
{
	//std::cout << "In: " << *x << " " << *y << std::endl;

	if (*x == 0 && *y == 0)
		return;

	float32 radius, angle;

	if (*x > fabs(*y)) { // Quadrant 1
		radius = *x;
		angle = QPI * (*y/ *x);
	} else if (*y > fabs(*x)) { // Quadrant 2
		radius = *y;
		angle = QPI * (2 - (*x/ *y));
	} else if (*x < -fabs(*y)) { // Quadrant 3
		radius = -*x;
		angle = QPI * (4 + (*y/ *x));
	} else { // Quadrant 4
		radius = -*y;
		angle = QPI * (6 - (*x/ *y));
	}

	*x = radius * cos(angle);
	*y = radius * sin(angle);

	//std::cout << "Out: " << *x << " " << *y << std::endl;
}

static inline Vec3 cosine_sample_hemisphere(float32 u, float32 v)
{
	Vec3 ret(u, v, 0.f);
	square_to_circle(&(ret.x), &(ret.y));
	ret.z = std::sqrt(std::max(0.f, 1.f - (ret.x*ret.x) - (ret.y*ret.y)));
	return ret;
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
 * Simple mapping of a vector that exists in a z-up space to
 * the space of another vector who's direction is considered
 * z-up for the purpose.
 * Obviously this doesn't care about the direction _around_
 * the z-up, although it will be sufficiently consistent for
 * most purposes.
 *
 * @param from The vector we're transforming.
 * @param toz The vector whose space we are transforming "from" into.
 *
 * @return The transformed vector.
 */
static inline Vec3 zup_to_vec(Vec3 from, Vec3 toz)
{
	toz.normalize();

	// Find the smallest axis of "to"
	int li = 0;
	float32 least = std::abs(toz.x);
	if (least > std::abs(toz.y)) {
		least = std::abs(toz.y);
		li = 1;
	}
	if (least > std::abs(toz.z))
		li = 2;

	// Find a perpendicular vector with "to"
	Vec3 tox(0.0, 0.0, 0.0);
	tox[li] = 1.0;
	tox = cross(toz, tox);
	tox.normalize();

	// Find a third vector perpendicular to both
	Vec3 toy = cross(toz, tox);
	toy.normalize();

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
