#ifndef MONTE_CARLO_HPP
#define MONTE_CARLO_HPP

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

template <typename T>
static inline Vec3 uniform_sample_cone(T u, T v, T cos_theta_max)
{
	const T cos_theta = (1.0 - u) + (u * cos_theta_max);
	const T sin_theta = std::sqrt(1.0 - (cos_theta * cos_theta));
	const T phi = v * 2.0 * M_PI;
	return Vec3(std::cos(phi) * sin_theta, std::sin(phi) * sin_theta, cos_theta);
}

template <typename T>
static inline T uniform_sample_cone_pdf(T cos_theta_max)
{
	// 1.0 / solid angle
	return 1.0 / (2.0 * M_PI * (1.0 - cos_theta_max));
}

#endif // MONTE_CARLO_HPP