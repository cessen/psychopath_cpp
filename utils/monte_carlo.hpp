#ifndef MONTE_CARLO_HPP
#define MONTE_CARLO_HPP

#include "utils.hpp"

/**
 * The logit function, scaled to approximate the probit function.
 *
 * We use this as a close approximation to the gaussian inverse CDF,
 * since the gaussian inverse CDF (probit) has no analytic formula.
 */
static inline float logit(float p, float width = 1.5f) {
	p = 0.001f + (p * 0.998f);
	return logf(p/(1.0f-p)) * width * (0.6266f/4.0f);
}

static inline float fast_logit(float p, float width = 1.5f) {
	const float n = 0.001f + (p * 0.998f);

	return faster_ln((n / (1.0f - n))) * width * (0.6266f / 4.0f);
}

/*
 * Maps the unit square to the unit circle.
 * Modifies x and y in place.
 * NOTE: x and y should be distributed within [-1, 1],
 * not [0, 1].
 */
static inline void square_to_circle(float *x, float *y) {
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

static inline Vec3 cosine_sample_hemisphere(float u, float v) {
	u = (u*2)-1;
	v = (v*2)-1;
	square_to_circle(&u, &v);
	const float z = std::sqrt(std::max(0.0, 1.0 - ((u*u) + (v*v))));
	return Vec3(u, v, z);
}

static inline Vec3 cosine_sample_hemisphere_polar(float u, float v) {
	const float r = std::sqrt(u);
	const float theta = 2 * M_PI * v;

	const float x = r * std::cos(theta);
	const float y = r * std::sin(theta);

	return Vec3(x, y, std::sqrt(std::max(0.0f, 1 - u)));
}

static inline Vec3 uniform_sample_hemisphere(float u, float v) {
	float z = u;
	float r = std::sqrt(std::max(0.f, 1.f - z*z));
	float phi = 2 * M_PI * v;
	float x = r * std::cos(phi);
	float y = r * std::sin(phi);
	return Vec3(x, y, z);
}

static inline Vec3 uniform_sample_sphere(float u, float v) {
	float z = 1.f - (2.f * u);
	float r = std::sqrt(std::max(0.f, 1.f - z*z));
	float phi = 2.f * M_PI * v;
	float x = r * std::cos(phi);
	float y = r * std::sin(phi);
	return Vec3(x, y, z);
}

template <typename T>
static inline Vec3 uniform_sample_cone(T u, T v, T cos_theta_max) {
	const T cos_theta = (1.0 - u) + (u * cos_theta_max);
	const T sin_theta = std::sqrt(1.0 - (cos_theta * cos_theta));
	const T phi = v * 2.0 * M_PI;
	return Vec3(std::cos(phi) * sin_theta, std::sin(phi) * sin_theta, cos_theta);
}

template <typename T>
static inline T uniform_sample_cone_pdf(T cos_theta_max) {
	// 1.0 / solid angle
	return 1.0 / (2.0 * M_PI * (1.0 - cos_theta_max));
}

/**
 * Calculates the projected solid angle of a spherical triangle.
 *
 * A, B, and C are the points of the triangle on a unit sphere.
 */
static inline float spherical_triangle_solid_angle(Vec3 A, Vec3 B, Vec3 C) {
	// Calculate sines and cosines of the spherical triangle's edge lengths
	const double cos_a = clamp((double)(dot(B, C)), -1.0, 1.0);
	const double cos_b = clamp((double)(dot(C, A)), -1.0, 1.0);
	const double cos_c = clamp((double)(dot(A, B)), -1.0, 1.0);
	const double sin_a = std::sqrt(1.0 - (cos_a * cos_a));
	const double sin_b = std::sqrt(1.0 - (cos_b * cos_b));
	const double sin_c = std::sqrt(1.0 - (cos_c * cos_c));

	// If two of the vertices are coincident, area is zero.
	// Return early to avoid a divide by zero below.
	if (cos_a == 1.0 || cos_b == 1.0 || cos_c == 1.0) {
		return 0.0;
	}

	// Calculate the cosine of the angles at the vertices
	const double cos_A = clamp((cos_a - (cos_b * cos_c)) / (sin_b * sin_c), -1.0, 1.0);
	const double cos_B = clamp((cos_b - (cos_c * cos_a)) / (sin_c * sin_a), -1.0, 1.0);
	const double cos_C = clamp((cos_c - (cos_a * cos_b)) / (sin_a * sin_b), -1.0, 1.0);

	// Calculate the angles themselves, in radians
	const double ang_A = std::acos(cos_A);
	const double ang_B = std::acos(cos_B);
	const double ang_C = std::acos(cos_C);

	// Calculate and return the solid angle of the triangle
	return ang_A + ang_B + ang_C - M_PI;
}

/**
 * Generates a uniform sample on a spherical triangle given two uniform
 * random variables i and j in [0, 1].
 */
static inline Vec3 uniform_sample_spherical_triangle(Vec3 A, Vec3 B, Vec3 C, float i, float j) {
	// Calculate sines and cosines of the spherical triangle's edge lengths
	const double cos_a = clamp((double)(dot(B, C)), -1.0, 1.0);
	const double cos_b = clamp((double)(dot(C, A)), -1.0, 1.0);
	const double cos_c = clamp((double)(dot(A, B)), -1.0, 1.0);
	const double sin_a = std::sqrt(1.0 - (cos_a * cos_a));
	const double sin_b = std::sqrt(1.0 - (cos_b * cos_b));
	const double sin_c = std::sqrt(1.0 - (cos_c * cos_c));

	// If two of the vertices are coincident, area is zero.
	// Return early to avoid a divide by zero below.
	if (cos_a == 1.0 || cos_b == 1.0 || cos_c == 1.0) {
		// TODO: do something more intelligent here, in the case that it's
		// an infinitely thin line.
		return A;
	}

	// Calculate the cosine of the angles at the vertices
	const double cos_A = clamp((cos_a - (cos_b * cos_c)) / (sin_b * sin_c), -1.0, 1.0);
	const double cos_B = clamp((cos_b - (cos_c * cos_a)) / (sin_c * sin_a), -1.0, 1.0);
	const double cos_C = clamp((cos_c - (cos_a * cos_b)) / (sin_a * sin_b), -1.0, 1.0);

	// Calculate sine for A
	const double sin_A = std::sqrt(1.0 - (cos_A * cos_A));

	// Calculate the angles themselves, in radians
	const double ang_A = std::acos(cos_A);
	const double ang_B = std::acos(cos_B);
	const double ang_C = std::acos(cos_C);

	// Calculate the area of the spherical triangle
	const double area = ang_A + ang_B + ang_C - M_PI;

	// The rest of this is from the paper "Stratified Sampling of Spherical
	// Triangles" by James Arvo.
	const double area_2 = area * i;

	const double s = std::sin(area_2 - ang_A);
	const double t = std::cos(area_2 - ang_A);
	const double u = t - cos_A;
	const double v = s + (sin_A * cos_c);

	const double q_top = (((v * t) - (u * s)) * cos_A) - v;
	const double q_bottom = ((v * s) + (u * t)) * sin_A;
	const double q = q_top / q_bottom;

	const Vec3 C_2 = (A * q) + ((C - (A * dot(C, A))).normalized() * std::sqrt(1.0 - (q*q)));

	const double z = 1.0 - (j * (1.0 - dot(C_2, B)));

	const Vec3 result = (B * z) + ((C_2 - (B * dot(C_2, B))).normalized() * std::sqrt(1.0 - (z*z)));

	return result;
}


/**
 * Analytically calculates lambert shading from a uniform light source
 * subtending a circular solid angle.
 * Only works for solid angle subtending equal to or less than a hemisphere.
 *
 * Formula taken from "Area Light Sources for Real-Time Graphics"
 * by John M. Snyder
 */
static inline float sphere_lambert(float nlcos, float rcos) {
	assert(nlcos >= -1.0 && nlcos <= 1.0);
	assert(rcos >= 0.0 && rcos <= 1.0);

	const float nlsin = std::sqrt(1.0 - (nlcos * nlcos));
	const float rsin2 = 1.0 - (rcos * rcos);
	const float rsin = std::sqrt(rsin2);
	const float ysin = rcos / nlsin;
	const float ycos2 = 1.0 - (ysin * ysin);
	const float ycos = std::sqrt(ycos2);

	const float g = (-2.0 * nlsin * rcos * ycos) + HPI - std::asin(ysin) + (ysin * ycos);
	const float h = nlcos * ((ycos * std::sqrt(rsin2 - ycos2)) + (rsin2 * std::asin(ycos / rsin)));

	const float nl = std::acos(nlcos);
	const float r = std::acos(rcos);

	if (nl < (HPI - r)) {
		return nlcos * rsin2;
	} else if (nl < HPI) {
		return (nlcos * rsin2) + g - h;
	} else if (nl < (HPI + r)) {
		return (g + h) * INV_PI;
	} else {
		return 0.0;
	}
}

#endif // MONTE_CARLO_HPP