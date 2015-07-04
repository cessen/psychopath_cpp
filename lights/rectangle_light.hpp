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
	 */
	virtual SpectralSample sample(const Vec3 &arr, float u, float v, float wavelength, float time, Vec3 *shadow_vec, float* pdf) const override {
		// Calculate time interpolated values
		const auto dim = lerp_seq(time, dimensions);
		const double inv_surface_area = 1.0 / (dim.first * dim.second);
		const Color col = lerp_seq(time, colors);

		// Get the four corners of the rectangle, projected on to the unit
		// sphere centered around arr.
		const Vec3 p1 = (Vec3(dim.first * 0.5f, dim.second * 0.5f, 0.0f) - arr).normalized();
		const Vec3 p2 = (Vec3(dim.first * -0.5f, dim.second * 0.5f, 0.0f) - arr).normalized();
		const Vec3 p3 = (Vec3(dim.first * -0.5f, dim.second * -0.5f, 0.0f) - arr).normalized();
		const Vec3 p4 = (Vec3(dim.first * 0.5f, dim.second * -0.5f, 0.0f) - arr).normalized();

		// Get the solid angles of the rectangle split into two triangles
		const float area_1 = spherical_triangle_solid_angle(p2, p1, p3);
		const float area_2 = spherical_triangle_solid_angle(p4, p1, p3);

		// Normalize the solid angles for selection purposes
		const float prob_1 = area_1 / (area_1 + area_2);
		const float prob_2 = 1.0f - prob_1;

		// Select one of the triangles and sample it
		if (u < prob_1) {
			*shadow_vec = uniform_sample_spherical_triangle(p2, p1, p3, v, u / prob_1);
		} else {
			*shadow_vec = uniform_sample_spherical_triangle(p4, p1, p3, v, 1.0f - ((u - prob_1) / prob_2));
		}

		// Project shadow_vec back onto the light's surface
		*shadow_vec *= -arr.z / shadow_vec->z;

		*pdf = 1.0f / (area_1 + area_2); // PDF of the ray direction being sampled

		return Color_to_SpectralSample(col * inv_surface_area * 0.5f, wavelength); // 0.5x because it emits on both sides
	}

	virtual float sample_pdf(const Vec3 &arr, const Vec3 &sample_dir, float sample_u, float sample_v, float wavelength, float time) const override {
		// Calculate time interpolated values
		const auto dim = lerp_seq(time, dimensions);

		// Get the four corners of the rectangle, projected on to the unit
		// sphere centered around arr.
		const Vec3 p1 = (Vec3(dim.first * 0.5f, dim.second * 0.5f, 0.0f) - arr).normalized();
		const Vec3 p2 = (Vec3(dim.first * -0.5f, dim.second * 0.5f, 0.0f) - arr).normalized();
		const Vec3 p3 = (Vec3(dim.first * -0.5f, dim.second * -0.5f, 0.0f) - arr).normalized();
		const Vec3 p4 = (Vec3(dim.first * 0.5f, dim.second * -0.5f, 0.0f) - arr).normalized();

		// Get the solid angles of the rectangle split into two triangles
		const float area_1 = spherical_triangle_solid_angle(p2, p1, p3);
		const float area_2 = spherical_triangle_solid_angle(p4, p1, p3);

		return 1.0f / (area_1 + area_2);
	}

	virtual SpectralSample outgoing(const Vec3 &dir, float u, float v, float wavelength, float time) const override {
		const auto dim = lerp_seq(time, dimensions);
		const double surface_area = (dim.first * dim.second);
		const Color col = lerp_seq(time, colors);
		return Color_to_SpectralSample(col / surface_area * 0.5f, wavelength); // 0.5x because it emits on both sides
	}

	virtual bool is_delta() const override {
		return false;
	}

	virtual Color total_emitted_color() const override {
		Color col = lerp_seq(0, colors);
		return col;
	}

	virtual bool intersect_ray(const Ray &ray, Intersection *intersection=nullptr) const override {
		if (ray.d.z == 0.0f) {
			return false;
		}

		const auto dim = lerp_seq(ray.time, dimensions);

		const float t = -ray.o.z * ray.d_inv.z;

		if (t <= 0.0f || t > ray.max_t) {
			return false;
		}

		const float x = ray.o.x + (ray.d.x * t);
		const float y = ray.o.y + (ray.d.y * t);

		// Check if we hit
		if (x >= (dim.first * -0.5f) && x <= (dim.first * 0.5f) && y >= (dim.second * -0.5f) && y <= (dim.first * 0.5f)) {
			intersection->t = t;

			intersection->geo.p = Vec3(x, y, 0.0f);
			intersection->geo.n = Vec3(0.0f, 0.0f, 1.0f);

			intersection->backfacing = ray.d.z > 0.0f;

			intersection->light_pdf = sample_pdf(ray.o, ray.d, 0.0f, 0.0f, 0.0f, ray.time);

			intersection->offset = intersection->geo.n * 0.000001f;

			const double surface_area = dim.first * dim.second;
			const Color col = lerp_seq(ray.time, colors) * 0.5f / surface_area;
			intersection->surface_closure.init(EmitClosure(col));

			return true;
		} else {
			return false;
		}
	}

	virtual const std::vector<BBox>& bounds() const override {
		return bounds_;
	}
};

#endif // RECTANGLE_LIGHT_HPP
