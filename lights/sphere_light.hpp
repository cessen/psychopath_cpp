#ifndef SPHERE_LIGHT_HPP
#define SPHERE_LIGHT_HPP

#include "light.hpp"
#include "utils.hpp"
#include "monte_carlo.hpp"
#include <limits>
#include <cmath>
#include <algorithm>

/**
 * @brief A spherical light source, emitting light evenly from its surface.
 */
class SphereLight final: public Light
{
	std::vector<Vec3> positions;
	std::vector<float> radii;
	std::vector<Color> colors;
	std::vector<BBox> bounds_;

public:
	SphereLight(std::vector<Vec3>& positions_, std::vector<float>& radii_, std::vector<Color>& colors_): positions {positions_}, radii {radii_}, colors {colors_} {
		// Check for missing info
		if (positions.size() == 0)
			positions = {Vec3(0,0,0)};
		if (radii.size() == 0)
			std::cout << "WARNING: SphereLight has no radius(s)!\n";
		if (colors.size() == 0)
			std::cout << "WARNING: SphereLight has no color(s)!\n";

		// Fill in bounds
		bounds_.clear();
		if (positions.size() >= radii.size()) {
			const float s = std::max(positions.size() - 1, 1UL);
			for (unsigned int i = 0; i < positions.size(); ++i) {
				const Vec3& pos = positions[i];
				const Vec3 rad3 {
					lerp_seq(i/s, radii)
				};
				bounds_.emplace_back(pos - rad3, pos + rad3);
			}
		} else {
			const float s = std::max(radii.size() - 1, 1UL);
			for (unsigned int i = 0; i < radii.size(); ++i) {
				const Vec3 pos {
					lerp_seq(i/s, positions)
				};
				const Vec3 rad3 {
					radii[i], radii[i], radii[i]
				};
				bounds_.emplace_back(pos - rad3, pos + rad3);
			}
		}
	}

	/**
	 * Samples the sphere light from a given point.
	 */
	virtual SpectralSample sample(const Vec3 &arr, float u, float v, float wavelength, float time, Vec3 *shadow_vec, float* pdf) const override {
		// Calculate time interpolated values
		Vec3 pos = lerp_seq(time, positions);
		double radius = lerp_seq(time, radii);
		Color col = lerp_seq(time, colors);
		double surface_area_inv = 1.0 / (4.0 * M_PI * radius * radius);


		// Create a coordinate system from the vector between the
		// point and the center of the light
		Vec3 x, y, z;
		z = pos - arr;
		const double d2 = z.length2();  // Distance from center of sphere squared
		const double d = std::sqrt(d2); // Distance from center of sphere
		coordinate_system_from_vec3(z, &x, &y);
		x.normalize();
		y.normalize();
		z.normalize();

		// If we're outside the sphere, sample the surface based on
		// the angle it subtends from the point being lit.
		if (d > radius) {
			// Calculate the portion of the sphere visible from the point
			const double sin_theta_max2 = std::min(1.0, (static_cast<double>(radius) * static_cast<double>(radius)) / d2);
			const double cos_theta_max2 = 1.0 - sin_theta_max2;
			const double sin_theta_max = std::sqrt(sin_theta_max2);
			const double cos_theta_max = std::sqrt(cos_theta_max2);

			// Sample the cone subtended by the sphere
			Vec3 sample = uniform_sample_cone(u, v, cos_theta_max).normalized();

			// Find the intersection of the sample ray with the sphere, and
			// scale the sample ray to match the intersection distance.
			{
				// Calculate quadratic coeffs
				Vec3 oo {0.0f, 0.0f, (float)(-d)};
				const float a = sample.length2();
				const float b = 2.0f * dot(sample, oo);
				const float c = oo.length2() - radius * radius;

				float t0, t1, discriminant;
				discriminant = b * b - 4.0f * a * c;
				if (discriminant < 0.0f) {
					// Discriminant less than zero?  No solution => no intersection.
					// Assume the sample is on the edge, and use the subtending disc
					// distance.
					const double disc_radius = cos_theta_max * radius;
					const double disc_dist = d - (sin_theta_max * radius);
					const double length = std::sqrt((disc_dist * disc_dist) + (disc_radius * disc_radius));
					sample *= length;
				} else {
					discriminant = std::sqrt(discriminant);

					// Compute a more stable form of our param t (t0 = q/a, t1 = c/q)
					// q = -0.5 * (b - sqrt(b * b - 4.0 * a * c)) if b < 0, or
					// q = -0.5 * (b + sqrt(b * b - 4.0 * a * c)) if b >= 0
					float q;
					if (b < 0.0f) {
						q = -0.5f * (b - discriminant);
					} else {
						q = -0.5f * (b + discriminant);
					}

					// Get our final parametric values
					t0 = q / a;
					if (q != 0.0f) {
						t1 = c / q;
					} else {
						t1 = std::numeric_limits<float>::infinity();
					}

					// Adjust the sample ray distance to match the
					// intersection distance
					if (t0 <= t1) {
						sample *= t0;
					} else {
						sample *= t1;
					}
				}
			}

			// Transform the ray into the proper space, with the proper length
			*shadow_vec = (x * sample[0]) + (y * sample[1]) + (z * sample[2]);

			*pdf = uniform_sample_cone_pdf(cos_theta_max);
			return Color_to_SpectralSample(col * surface_area_inv, wavelength);
		} else {
			// If we're inside the sphere, there's light from every direction.
			*shadow_vec = uniform_sample_sphere(u, v);
			*pdf = 1.0f / (4.0f * M_PI);
			return Color_to_SpectralSample(col * surface_area_inv, wavelength);
		}

	}

	virtual float sample_pdf(const Vec3 &arr, const Vec3 &sample_dir, float sample_u, float sample_v, float wavelength, float time) const override {
		Vec3 pos = lerp_seq(time, positions);
		double radius = lerp_seq(time, radii);

		const double d2 = (pos - arr).length2();  // Distance from center of sphere squared
		const double d = std::sqrt(d2); // Distance from center of sphere

		if (d > radius) {
			// Calculate the portion of the sphere visible from the point
			const double sin_theta_max2 = std::min(1.0, (static_cast<double>(radius) * static_cast<double>(radius)) / d2);
			const double cos_theta_max2 = 1.0 - sin_theta_max2;
			const double cos_theta_max = std::sqrt(cos_theta_max2);

			return uniform_sample_cone_pdf(cos_theta_max);
		} else {
			return 1.0f / (4.0f * M_PI);
		}
	}

	virtual SpectralSample outgoing(const Vec3 &dir, float u, float v, float wavelength, float time) const override {
		double radius = lerp_seq(time, radii);
		Color col = lerp_seq(time, colors);
		double surface_area = 4.0 * M_PI * radius * radius;
		return Color_to_SpectralSample(col / surface_area, wavelength);
	}

	virtual bool is_delta() const override {
		return false;
	}

	virtual Color total_emitted_color() const override {
		Color col = lerp_seq(0, colors);
		return col;
	}

	virtual bool intersect_ray(const Ray &ray, Intersection *intersection=nullptr) const override {
		// Get the center and radius of the sphere at the ray's time
		const Vec3 cent = lerp_seq(ray.time, positions); // Center of the sphere
		const float radi = lerp_seq(ray.time, radii); // Radius of the sphere
		double surface_area = 4.0 * M_PI * radi * radi;

		// Calculate the relevant parts of the ray for the intersection
		Vec3 o = ray.o - cent; // Ray origin relative to sphere center
		Vec3 d = ray.d;


		// Code taken shamelessly from https://github.com/Tecla/Rayito
		// Ray-sphere intersection can result in either zero, one or two points
		// of intersection.  It turns into a quadratic equation, so we just find
		// the solution using the quadratic formula.  Note that there is a
		// slightly more stable form of it when computing it on a computer, and
		// we use that method to keep everything accurate.

		// Calculate quadratic coeffs
		float a = d.length2();
		float b = 2.0f * dot(d, o);
		float c = o.length2() - radi * radi;

		float t0, t1, discriminant;
		discriminant = b * b - 4.0f * a * c;
		if (discriminant < 0.0f) {
			// Discriminant less than zero?  No solution => no intersection.
			return false;
		}
		discriminant = std::sqrt(discriminant);

		// Compute a more stable form of our param t (t0 = q/a, t1 = c/q)
		// q = -0.5 * (b - sqrt(b * b - 4.0 * a * c)) if b < 0, or
		// q = -0.5 * (b + sqrt(b * b - 4.0 * a * c)) if b >= 0
		float q;
		if (b < 0.0f) {
			q = -0.5f * (b - discriminant);
		} else {
			q = -0.5f * (b + discriminant);
		}

		// Get our final parametric values
		t0 = q / a;
		if (q != 0.0f) {
			t1 = c / q;
		} else {
			t1 = ray.max_t;
		}

		// Swap them so they are ordered right
		if (t0 > t1) {
			float temp = t1;
			t1 = t0;
			t0 = temp;
		}

		// Check our intersection for validity against this ray's extents
		if (t0 >= ray.max_t || t1 < 0.0001f)
			return false;

		float t;
		if (t0 >= 0.0001f) {
			t = t0;
		} else if (t1 < ray.max_t) {
			t = t1;
		} else {
			return false;
		}

		if (intersection && !ray.is_occlusion()) {
			intersection->t = t;

			intersection->geo.p = ray.o + (ray.d * t);
			intersection->geo.n = intersection->geo.p - cent;
			intersection->geo.n.normalize();

			intersection->backfacing = dot(intersection->geo.n, ray.d.normalized()) > 0.0f;

			intersection->light_pdf = sample_pdf(ray.o, ray.d, 0.0f, 0.0f, 0.0f, ray.time);

			intersection->offset = intersection->geo.n * 0.000001f;

			const Color col = lerp_seq(ray.time, colors) / surface_area;
			intersection->surface_closure.init(EmitClosure(col));
		}

		return true;
	}

	virtual const std::vector<BBox>& bounds() const override {
		return bounds_;
	}
};

#endif // SPHERE_LIGHT_HPP
