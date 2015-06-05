#ifndef SPHERE_LIGHT_HPP
#define SPHERE_LIGHT_HPP

#include "light.hpp"
#include "utils.hpp"
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
	 *
	 * Note that shadowing will only be 100% correct as long as there are no
	 * blockers inside the sphere light, because the generated ray sometimes
	 * ends inside the sphere rather than on the surface.  In general this is
	 * unlikely to cause noticable artifacts.  But it's worth noting.
	 *
	 * TODO: fix the above note whenever you get around to implementing volume
	 * rendering, as it may meaningfully impact lighting effects from a sphere
	 * light embedded inside a volume.
	 */
	virtual Color sample(const Vec3 &arr, float u, float v, float time, Vec3 *shadow_vec, float* pdf) const override {
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

			// Calculate the length that the shadow ray should be.
			// TODO: make the length end exactly on the surface of
			// the sphere.
			const double disc_radius = cos_theta_max * radius;
			const double disc_dist = d - (sin_theta_max * radius);
			const double length = std::sqrt((disc_dist * disc_dist) + (disc_radius * disc_radius));

			// Sample the cone subtended by the sphere
			Vec3 sample = uniform_sample_cone(u, v, cos_theta_max);
			*shadow_vec = ((x * sample[0]) + (y * sample[1]) + (z * sample[2])).normalized() * length;

			*pdf = uniform_sample_cone_pdf(cos_theta_max);
			return col * surface_area_inv;
		} else {
			// If we're inside the sphere, there's light from every direction.
			*shadow_vec = uniform_sample_sphere(u, v);
			*pdf = 1.0f / (4.0f * M_PI);
			return col * surface_area_inv;
		}

	}

	virtual Color outgoing(const Vec3 &dir, float u, float v, float time) const override {
		double radius = lerp_seq(time, radii);
		Color col = lerp_seq(time, colors);
		double surface_area = 4.0 * M_PI * radius * radius;
		return col / surface_area;
	}

	virtual bool is_delta() const override {
		return false;
	}

	virtual Color total_emitted_color() const override {
		Color col = lerp_seq(0, colors);
		return col;
	}

	virtual bool intersect_ray(const Ray &ray, Intersection *intersection=nullptr) const override {
		return false;
	}

	virtual const std::vector<BBox>& bounds() const override {
		return bounds_;
	}
};

#endif // SPHERE_LIGHT_HPP
