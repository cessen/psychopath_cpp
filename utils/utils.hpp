#ifndef UTILS_HPP
#define UTILS_HPP

#include "numtype.h"

#include "vector.hpp"

#include <iterator>
#include <cmath>
#include <cstring>
#include <cassert>
#include <tuple>

// Useful constants
#ifndef M_PI
#define M_PI 3.1415926535897932384626433
#endif
#define INV_PI (1.0 / M_PI)
#define QPI (M_PI / 4)

// Math operators for some types commonly used in Psychopath
static inline std::pair<float, float> operator *(const std::pair<float, float>& a, float b)
{
	return std::make_pair(a.first * b, a.second * b);
}

static inline std::pair<float, float> operator /(const std::pair<float, float>& a, float b)
{
	return std::make_pair(a.first / b, a.second / b);
}

static inline std::pair<float, float> operator +(const std::pair<float, float>& a, const std::pair<float, float>& b)
{
	return std::make_pair(a.first + b.first, a.second + b. second);
}

static inline std::pair<float, float> operator -(const std::pair<float, float>& a, const std::pair<float, float>& b)
{
	return std::make_pair(a.first - b.first, a.second - b. second);
}


// Returns value clamped to within the range [a,b]
template <class T>
static inline T clamp(const T& value, const T& a, const T& b)
{
	return std::min(b, std::max(a, value));
}

/*
   linear interpolation
   alpha = 0.0 means full a
   alpha = 1.0 means full b
 */
template <class T>
static inline T lerp(const float &alpha, const T &a, const T &b)
{
	assert(alpha >= 0 && alpha <= 1);
	return (a * (1.0-alpha)) + (b * alpha);
}


template <class T>
static inline T lerp2d(float alpha_u, float alpha_v,
                       T s00, T s10, T s01, T s11)
{
	const T temp1 = lerp(alpha_u, s00, s10);
	const T temp2 = lerp(alpha_u, s01, s11);
	return lerp(alpha_v, temp1, temp2);
}


/**
 * Performs a linear interpolation across a sequence of elements,
 * treating the sequence as a series of equally spaced linear
 * segments.  The sequence to interpolate is specified by a pair
 * of random-access iterators.
 *
 * alpha = 0.0 means the first element in the sequence
 * alpha = 1.0 means the last element in the sequence
 */
template<typename RandIt, typename T = typename std::iterator_traits<RandIt>::value_type>
T lerp_seq(float alpha, const RandIt& seq, int seq_length)
{
	assert(alpha >= 0 && alpha <= 1);
	assert(seq_length > 0);

	if (seq_length == 1) {
		return seq[0];
	} else {
		const float temp = alpha * (seq_length - 1);
		const auto index = static_cast<int>(temp);
		const float nalpha = temp - index;
		return lerp(nalpha, seq[index], seq[index+1]);
	}
}

template<typename RandIt, typename T = typename std::iterator_traits<RandIt>::value_type>
T lerp_seq(float alpha, const RandIt& begin, const RandIt& end)
{
	const auto seq_length = std::distance(begin, end);
	return lerp_seq(alpha, begin, seq_length);
}

// Version of lerp_seq for containers with begin() and end() methods that return iterators.
template<typename RandContainer, typename T = typename RandContainer::value_type>
T lerp_seq(float alpha, const RandContainer& c)
{
	return lerp_seq(alpha, c.cbegin(), c.cend());
}


/**
 * Partitions a range of elements based on a unary predicate function.
 *
 * This function applies the predicate precisely once to every element
 * in the range, which means it is safe and predictable for the predicate
 * to modify the elements of the list.
 *
 * By contrast, the predicate passed to std::partition is explicitly not
 * supposed to modify the elements.  In practice it works fine, but it
 * seems best to provide an implementation designed for that use in case
 * some platform's STL does weird things based on the assumption that the
 * predicate doesn't modify anything.
 *
 * This implementation is based on the implementation in libc++, which is
 * under the MIT open source license.
 */
template <typename Predicate, typename BidirIt>
BidirIt mutable_partition(BidirIt begin, BidirIt end, Predicate pred)
{
	alignas(decltype(*begin)) char tmp[sizeof(decltype(*begin))];

	while (true) {
		while (true) {
			if (begin == end)
				return begin;
			if (!pred(*begin))
				break;
			++begin;
		}

		do {
			if (begin == --end)
				return begin;
		} while (!pred(*end));

		std::memcpy((void*)&(*tmp), (void*)&(*begin), sizeof(decltype(*begin)));
		std::memcpy((void*)&(*begin), (void*)&(*end), sizeof(decltype(*begin)));
		std::memcpy((void*)&(*end), (void*)&(*tmp), sizeof(decltype(*begin)));

		++begin;
	}
}

/*template <typename Predicate, typename BidirIt>
BidirIt mutable_partition(BidirIt begin, BidirIt end, Predicate pred)
{
    alignas(decltype(*begin)) char tmp[sizeof(decltype(*begin))];

    auto last = begin;

    while (true) {
        if( begin == end)
            return last;
        if (!pred(*begin))
            break;
        ++begin;
        ++last;
    }
    ++begin;

    while (true) {
        while (true) {
            if( begin == end)
                return last;
            if (pred(*begin))
                break;
            ++begin;
        }

        std::memcpy((void*)&(*tmp), (void*)&(*begin), sizeof(decltype(*begin)));
        std::memcpy((void*)&(*begin), (void*)&(*last), sizeof(decltype(*begin)));
        std::memcpy((void*)&(*last), (void*)&(*tmp), sizeof(decltype(*begin)));

        ++last;
        ++begin;
    }

    return last;
}*/


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


/**
 * The logit function, scaled to approximate the probit function.
 *
 * We use this as a close approximation to the gaussian inverse CDF,
 * since the gaussian inverse CDF (probit) has no analytic formula.
 */
static inline float logit(float p, float width = 1.5f)
{
	p = 0.001f + (p * 0.998f);
	return logf(p/(1.0f-p)) * width * (0.6266f/4);
}

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

static inline Vec3 uniform_sample_cone(float u, float v, float cos_theta_max)
{
	const float cos_theta = (1.0f - u) + (u * cos_theta_max);
	const float sin_theta = std::sqrt(1.0f - (cos_theta * cos_theta));
	const float phi = v * 2.0f * M_PI;
	return Vec3(std::cos(phi) * sin_theta, std::sin(phi) * sin_theta, cos_theta);
}

static inline float uniform_sample_cone_pdf(float cos_theta_max)
{
	// 1.0 / solid angle
	return 1.0f / (2.0f * M_PI * (1.0f - cos_theta_max));
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

static inline float log2(float x)
{
	return std::log(x) * 1.4426950408889634f;
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

/**
 * @brief Approximate 1/sqrt(n)
 */
static inline float fastrsqrt(float n)
{
	union {
		int32_t i;
		float y;
	};

	y = n;
	i = 0x5f3759df - (i >> 1);

	// One iteration of newton's method
	const float x = n * 0.5f;
	y = y * (1.5f - (x * y * y));

	return y;
}

/**
 * @brief Even more approximate (but faster) 1/sqrt(n)
 */
static inline float fasterrsqrt(float n)
{
	union {
		int32_t i;
		float y;
	};

	y = n;
	i = 0x5f3759df - (i >> 1);

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




/**
 * Finds the parameter t on the first ray where the two given rays are closest.
 *
 * o1 and d1 are the origin and direction of the first ray.
 * o2 and d2 are the origin and direction of the second ray.
 *
 * Returns the t parameter and distance as a tuple.
 */
static inline std::tuple<float, float> closest_ray_t(Vec3 o1, Vec3 d1, Vec3 o2, Vec3 d2)
{
	const Vec3 w = o1 - o2;

	const float a = dot(d1, d1);
	const float b = dot(d1, d2);
	const float c = dot(d2, d2);
	const float d = dot(d1, w);
	const float e = dot(d2, w);

	const float denom = (a * c) - (b * b);

	float t1, t2;
	if (denom < 0.00001f) {
		t1 = 0.0f;
		t2 = (b>c ? d/b : e/c);
	} else {
		t1 = ((b * e) - (c * d)) / denom;
		t2 = ((a * e) - (b * d)) / denom;
	}

	const float distance = ((o1 + (d1 * t1)) - (o2 + (d2 * t2))).length();

	return std::make_tuple(t1, distance);
}


/**
 * Finds the parameter t where rays are closest when both are at t.
 * This is subtly but importantly different from closest_ray_t() above.
 *
 * o1 and d1 are the origin and direction of the first ray.
 * o2 and d2 are the origin and direction of the second ray.
 *
 * Returns the t parameter and distance as a tuple.
 */
static inline std::tuple<float, float> closest_ray_t2(Vec3 o1, Vec3 d1, Vec3 o2, Vec3 d2)
{
	const Vec3 dd = d1 - d2;
	const float dd2 = dot(dd, dd);

	if (dd2 < 0.00001f) {
		return std::make_tuple(-1.0f, -1.0f);
	}

	const Vec3 w = o1 - o2;
	const float t = -dot(w, dd) / dd2;
	const float distance = ((o1 + (d1 * t)) - (o2 + (d2 * t))).length();

	return std::make_tuple(t, distance);
}


/**
 * Finds the shortest distance between a point and a line.
 *
 * p is the point.
 * o and d are the point in space and direction that define the line.
 */
static inline float point_line_distance(Vec3 p, Vec3 o, Vec3 d)
{
	const auto w = p - o;
	const auto n = d.normalized();

	return cross(n, w).length();
}


#endif
