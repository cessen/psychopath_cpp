#ifndef SPHERE_LIGHT_HPP
#define SPHERE_LIGHT_HPP

#include "light.hpp"
#include "utils.hpp"
#include <math.h>

/**
 * @brief A point light source.
 *
 * Super simple point light source.  Practically an example of how
 * to write a finite light source.
 */
class SphereLight: public Light
{
	Vec3 pos;
	float radius;
	Color col;
	std::vector<BBox> bounds_;
	float surface_area_inv;

public:
	SphereLight(Vec3 pos_, float radius_, Color col_): pos {pos_}, radius {radius_}, col {col_}, bounds_ {BBox(pos - Vec3(radius), pos + Vec3(radius))} {
		surface_area_inv = 1.0f / (4 * M_PI * radius * radius);
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
	virtual Color sample(const Vec3 &arr, float u, float v, float time, Vec3 *shadow_vec) const {
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

			// Calculate the solid angle the sphere takes up from the point
			const double solid_angle = 2 * M_PI * (1.0 - cos_theta_max);

			// Calculate the length that the shadow ray should be.
			// TODO: make the length end exactly on the surface of
			// the sphere.
			const double disc_radius = cos_theta_max * radius;
			const double disc_dist = d - (sin_theta_max * radius);
			const double length = std::sqrt((disc_dist * disc_dist) + (disc_radius * disc_radius));

			// Sample the cone subtended by the sphere
			Vec3 sample = uniform_sample_cone(u, v, cos_theta_max);
			*shadow_vec = ((x * sample[0]) + (y * sample[1]) + (z * sample[2])).normalized() * length;

			return col * static_cast<float>(solid_angle * surface_area_inv * (0.5 / M_PI));
		} else {
			// If we're inside the sphere, there's light from every direction.
			*shadow_vec = uniform_sample_sphere(u, v);
			return col * surface_area_inv;
		}

	}

	virtual Color outgoing(const Vec3 &dir, float u, float v, float time) const {
		return col;
	}

	virtual bool is_delta() const {
		return true;
	}

	virtual bool is_infinite() const {
		return false;
	}

	virtual float total_energy() const {
		return col.energy();
	}

	virtual std::vector<BBox>& bounds() {
		return bounds_;
	}
};

#endif // SPHERE_LIGHT_HPP
