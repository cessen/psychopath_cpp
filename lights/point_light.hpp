#ifndef POINT_LIGHT_HPP
#define POINT_LIGHT_HPP

#include "light.hpp"

/**
 * @brief A point light source.
 *
 * Super simple point light source.  Practically an example of how
 * to write a finite light source.
 */
class PointLight: public Light
{
	Vec3 pos;
	Color col;

public:
	PointLight(Vec3 pos_, Color col_) {
		pos = pos_;
		col = col_;
	}

	virtual Color sample(const Vec3 &arr, float u, float v, float time,
	                     Vec3 *shadow_vec) const {
		*shadow_vec = pos - arr;
		float d2 = shadow_vec->length2();
		if (d2 > 0)
			return col / d2;
		else
			return col; // Fudge for divide by zero.
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

	virtual BBox bounds() const {
		return BBox(pos, pos);
	}
};

#endif // POINT_LIGHT_HPP
