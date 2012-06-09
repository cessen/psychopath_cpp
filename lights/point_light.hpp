#ifndef POINT_LIGHT_HPP
#define POINT_LIGHT_HPP

#include "light.hpp"

/**
 * @brief A point light source.
 *
 * Super simple point light source.  Practically an example of how
 * to write a finite light source.
 */
class PointLight: public FiniteLight
{
	Vec3 pos;
	Color col;

public:
	PointLight(Vec3 pos_, Color col_) {
		pos = pos_;
		col = col_;
	}

	virtual Vec3 get_sample_position(float u, float v, float time) const {
		return pos;
	}

	virtual Color outgoing_light(Vec3 dir, float u, float v, float time) const {
		return col;
	}
};

#endif // POINT_LIGHT_HPP
