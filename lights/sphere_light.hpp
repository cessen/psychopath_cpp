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
	float32 radius;
	Color col;

public:
	SphereLight(Vec3 pos_, float32 radius_, Color col_) {
		pos = pos_;
		radius = radius_;
		col = col_;
	}

	virtual Color sample(const Vec3 &arr, float32 u, float32 v, float32 time,
	                     Vec3 *shadow_vec) const {
	    Vec3 n = uniform_sample_sphere(u, v);
	    Vec3 p = (n * radius) + pos;
	    
		*shadow_vec = p - arr;
		Vec3 out = *shadow_vec * -1.f;
		out.normalize();
		float d2 = shadow_vec->length2();
		
		// Convert to solid angle
		float ndot = std::abs(dot(n, out)) * 2.f;
		
		if (d2 > 0)
			return col * ndot / d2;
		else
			return col * ndot; // Fudge for divide by zero.
	}

	virtual Color outgoing(const Vec3 &dir, float32 u, float32 v, float32 time) const {
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
