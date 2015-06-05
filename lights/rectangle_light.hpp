#ifndef RECTANGLE_LIGHT_HPP
#define RECTANGLE_LIGHT_HPP

#include "light.hpp"
#include "utils.hpp"
#include <cmath>
#include <utility>
#include <algorithm>


/**
 * @brief A rectangular light source, emitting light evenly from its surface.
 */
class RectangleLight final: public Light
{
	std::vector<std::pair<float, float>> dimensions;
	std::vector<Color> colors;
	std::vector<BBox> bounds_;

public:
	RectangleLight(std::vector<std::pair<float, float>>& dimensions_, std::vector<Color>& colors_): dimensions {dimensions_}, colors {colors_} {
		// Check for missing info
		if (dimensions.size() == 0)
			std::cout << "WARNING: RectangleLight has no dimension(s)!\n";
		if (colors.size() == 0)
			std::cout << "WARNING: RectangleLight has no color(s)!\n";

		// Fill in bounds
		bounds_.clear();
		for (const auto& d :dimensions) {
			const Vec3 dim3 {
				d.first * 0.5f, d.second * 0.5f, 0.0f
			};
			bounds_.emplace_back(Vec3(0.0f) - dim3, Vec3(0.0f) + dim3);
		}
	}

	/**
	 * Samples the rectangle light from a given point.
	 *
	 * TODO: improve sampling to sample evenly within the projected solid
	 * angle.
	 */
	virtual Color sample(const Vec3 &arr, float u, float v, float time, Vec3 *shadow_vec, float* pdf) const override {
		// Calculate time interpolated values
		const auto dim = lerp_seq(time, dimensions);
		const double inv_surface_area = 1.0 / (dim.first * dim.second);
		const Color col = lerp_seq(time, colors);

		const Vec3 pos = Vec3 {
			(dim.first * u) - (dim.first * 0.5f),
			(dim.second * v) - (dim.second * 0.5f),
			0.0f
		};

		*shadow_vec = pos - arr;

		const float dist = shadow_vec->length();

		*pdf = (dist * dist) / std::abs(shadow_vec->normalized().z) * inv_surface_area; // PDF of the ray direction being sampled

		return col * inv_surface_area * 0.5f; // 0.5x because it emits on both sides
	}

	virtual Color outgoing(const Vec3 &dir, float u, float v, float time) const override {
		const auto dim = lerp_seq(time, dimensions);
		const double surface_area = (dim.first * dim.second);
		const Color col = lerp_seq(time, colors);
		return col / surface_area * 0.5f; // 0.5x because it emits on both sides
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

#endif // RECTANGLE_LIGHT_HPP
