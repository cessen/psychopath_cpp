#ifndef POINT_LIGHT_HPP
#define POINT_LIGHT_HPP

#include "light.hpp"

/**
 * @brief A point light source.
 *
 * Super simple point light source.  Practically an example of how
 * to write a finite light source.
 */
class PointLight final: public Light
{
	Vec3 pos;
	Color col;
	std::vector<BBox> bounds_;

public:
	PointLight(Vec3 pos_, Color col_): pos {pos_}, col {col_}, bounds_ {BBox(pos_, pos_)}
	{}

	virtual Color sample(const Vec3 &arr, float u, float v, float time,
	                     Vec3 *shadow_vec, float* pdf) const override {
		*pdf = 1.0f;
		*shadow_vec = pos - arr;
		float d2 = shadow_vec->length2();
		if (d2 > 0)
			return col / d2;
		else
			return col; // Fudge for divide by zero.
	}

	virtual Color outgoing(const Vec3 &dir, float u, float v, float time) const override {
		return col;
	}

	virtual bool is_delta() const override {
		return true;
	}

	virtual Color total_emitted_color() const override {
		return col;
	}

	virtual bool intersect_ray(const Ray &ray, Intersection *intersection=nullptr) const override {
		return false;
	}

	virtual const std::vector<BBox> &bounds() const override {
		return bounds_;
	}
};

#endif // POINT_LIGHT_HPP
