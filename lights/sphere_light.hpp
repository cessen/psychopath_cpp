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

public:
	SphereLight(Vec3 pos_, float radius_, Color col_) {
		pos = pos_;
		radius = radius_;
		col = col_;
	}

	virtual Color sample(const Vec3 &arr, float u, float v, float time, Vec3 *shadow_vec) const {
		// Create a coordinate system from the vector between the
		// point and the center of the light
		Vec3 x, y, z;
		z = pos - arr;
		const float d2 = z.length2();
		const float d = std::sqrt(d2);
		coordinate_system_from_vec3(z, &x, &y);
		x.normalize();
		y.normalize();
		z.normalize();

		// If we're outside the sphere, sample the surface based on
		// the angle it subtends from the point being lit.
		if (d > radius) {
			// Sample the surface of the sphere
			Vec3 sample = uniform_sample_sphere_from_distance(radius, d, u, v);

			// Map the sample to world coordinates
			*shadow_vec = (x * sample[0]) + (y * sample[1]) + (z * sample[2]);

			const float sd2 = shadow_vec->length2();

			if (sd2 > 0)
				return col / sd2;
			else
				return col; // Fudge for divide by zero.

		} else {
			// If we're inside the sphere, there's no light.
			*shadow_vec = uniform_sample_sphere(u, v);
			return Color(0.0f);
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
};

#endif // SPHERE_LIGHT_HPP
