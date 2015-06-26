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

	virtual SpectralSample sample(const Vec3 &arr, float u, float v, float wavelength, float time,
	                              Vec3 *shadow_vec, float* pdf) const override {
		*pdf = 1.0f;
		*shadow_vec = pos - arr;
		float d2 = shadow_vec->length2();
		if (d2 > 0)
			return Color_to_SpectralSample(col / d2, wavelength);
		else
			return Color_to_SpectralSample(col, wavelength); // Fudge for divide by zero.
	}

	virtual float sample_pdf(const Vec3 &arr, const Vec3 &sample_dir, float sample_u, float sample_v, float wavelength, float time) const override {
		return 0.0f;
	}

	virtual SpectralSample outgoing(const Vec3 &dir, float u, float v, float wavelength, float time) const override {
		return Color_to_SpectralSample(col, wavelength);
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
