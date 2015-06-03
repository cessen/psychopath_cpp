#ifndef SURFACE_CLOSURE_HPP
#define SURFACE_CLOSURE_HPP

#include <cmath>

#include "numtype.h"
#include "utils.hpp"

#include "vector.hpp"
#include "color.hpp"
#include "ray.hpp"
#include "differential_geometry.hpp"

/**
 * Interface for surface closures.
 *
 * IMPORTANT: due to the way surface closures are used elsewhere in Psychopath,
 * any class inheriting from this MUST be a POD type (aside from the inheritance
 * and implementing this interface, of course).
 */
class SurfaceClosure
{
public:
	/**
	 * Returns whether the closure has a delta distribution or not.
	 */
	virtual bool is_delta() const = 0;

	/**
	 * Given an incoming ray and sample values, generates an outgoing ray and
	 * color filter.
	 *
	 * @param [in] in  Incoming light direction.
	 * @param [in] geo The differential geometry of the reflecting/transmitting surface point.
	 * @param [in] si  A sample value.
	 * @param [in] sj  A sample value.
	 *
	 * @param [out] out    The generated outgoing light direction.
	 * @param [out] filter The generated color filter.
	 * @param [out] pdf    The pdf value of the outgoing ray.
	 */
	virtual void sample(const Vec3 &in, const DifferentialGeometry &geo, const float &si, const float &sj,
	                    Vec3 *out, Color *filter, float *pdf) const = 0;


	/**
	 * Evaluates the closure for the given incoming and outgoing rays.
	 *
	 * @param [in] in  The incoming light direction.
	 * @param [in] out The outgoing light direction.
	 * @param [in] geo The differential geometry of the reflecting/transmitting surface point.
	 *
	 * @return The resulting filter color.
	 */
	virtual Color evaluate(const Vec3 &in, const Vec3 &out, const DifferentialGeometry &geo) const = 0;


	/**
	 * Transfers the ray differentials from 'in' to 'out' based on their
	 * incoming and outgoing directions and the differential geometry of the
	 * intersection point.
	 *
	 * @param [in] t   The t parameter along the 'in' ray's length where the intersection occured.
	 * @param [in] in  The incoming ray.
	 * @param [in] geo The differential geometry of the reflecting/transmitting surface point.
	 *
	 * @param [in out] out The outgoing ray.
	 */
	virtual void propagate_differentials(const float t, const WorldRay& in, const DifferentialGeometry &geo, WorldRay* out) const = 0;


	/**
	 * Returns the pdf for the given 'in' direction producing the given 'out'
	 * direction with the given differential geometry.
	 *
	 * @param [in] in  The incoming light direction.
	 * @param [in] out The outgoing light direction.
	 * @param [in] geo The differential geometry of the reflecting/transmitting surface point.
	 */
	virtual float pdf(const Vec3& in, const Vec3& out, const DifferentialGeometry &geo) const = 0;
};




/**
 * Emit closure.
 *
 * NOTE: this needs to be handled specially by the integrator!  It does not
 * behave like a standard closure!
 */
class EmitClosure final: public SurfaceClosure
{
	Color col {1.0f};

public:
	EmitClosure() = default;
	EmitClosure(Color col): col {col} {}


	virtual bool is_delta() const override {
		return false;
	}


	virtual void sample(const Vec3 &in, const DifferentialGeometry &geo, const float &si, const float &sj,
	                    Vec3 *out, Color *filter, float *pdf) const override {
		*pdf = 1.0f;
		*out = Vec3(1.0f);
		*filter = Color(0.0f);
	}


	Color evaluate(const Vec3 &in, const Vec3 &out, const DifferentialGeometry &geo) const override {
		return Color(0.0f);
	}


	void propagate_differentials(const float t, const WorldRay& in, const DifferentialGeometry &geo, WorldRay* out) const override {
		// Irrelivant
	}


	float pdf(const Vec3& in, const Vec3& out, const DifferentialGeometry &geo) const override {
		return 1.0f;
	}

	Color emitted_color() const {
		return col;
	}
};




class LambertClosure final: public SurfaceClosure
{
	Color col {1.0f};

public:
	LambertClosure() = default;
	LambertClosure(Color col): col {col} {}


	virtual bool is_delta() const override {
		return false;
	}


	virtual void sample(const Vec3 &in, const DifferentialGeometry &geo, const float &si, const float &sj,
	                    Vec3 *out, Color *filter, float *pdf) const override {
		// Get normalized surface normal
		Vec3 nn = geo.n.normalized();

		// If back-facing, flip normal
		if (dot(nn, in) > 0.0f) {
			nn *= -1.0f;
		}

		// Generate a random ray direction in the hemisphere
		// of the surface.
		const Vec3 dir = cosine_sample_hemisphere(si, sj);
		*pdf = dir.z * (float)(INV_PI) * 2.0f;
		*out = zup_to_vec(dir, nn);
		*filter = col * evaluate(in, *out, geo);
	}


	Color evaluate(const Vec3 &in, const Vec3 &out, const DifferentialGeometry &geo) const override {
		Vec3 nn = geo.n.normalized();
		const Vec3 v = out.normalized();

		if (dot(nn, in) > 0.0f)
			nn *= -1.0f;

		return col * std::max(dot(nn, v), 0.0f);
	}


	void propagate_differentials(const float t, const WorldRay& in, const DifferentialGeometry &geo, WorldRay* out) const override {
		const float len = out->d.length();
		const Vec3 nn = geo.n.normalized();

		Vec3 x, y;
		coordinate_system_from_vec3(out->d, &x, &y);
		x.normalize();
		y.normalize();

		out->odx = transfer_ray_origin_differential(t, nn, in.d, in.odx, in.ddx);
		out->ody = transfer_ray_origin_differential(t, nn, in.d, in.ody, in.ddy);
		out->ddx = x * 0.15f / len;
		out->ddy = y * 0.15f / len;
	}


	float pdf(const Vec3& in, const Vec3& out, const DifferentialGeometry &geo) const override {
		Vec3 nn = geo.n.normalized();
		const Vec3 v = out.normalized();

		if (dot(nn, in.normalized()) > 0.0f)
			nn *= -1.0f;

		return std::max(dot(nn, v) * (float)(INV_PI) * 2.0f, 0.0f);
	}
};




/**
 * The GTR microfacet BRDF from the Disney Principled BRDF paper.
 */
class GTRClosure final: public SurfaceClosure
{
private:
	Color col {0.903f};
	float roughness {0.05f};
	float tail_shape {2.0f};
	float fresnel {0.25f};
	float normalization_factor = normalization(roughness, tail_shape);



	// Makes sure values are in a valid range
	void validate() {
		// Clamp values to valid ranges
		roughness = std::max(0.0f, std::min(0.9999f, roughness));
		tail_shape = std::max(0.0001f, std::min(8.0f, tail_shape));

		// When roughness is too small, but not zero, there are floating point accuracy issues
		if (roughness < 0.000244140625f) // (2^-12)
			roughness = 0.0f;

		// If tail_shape is too near 1.0, push it away a tiny bit.
		// This avoids having to have a special form of various equations
		// due to a singularity at tail_shape = 1.0
		// That in turn avoids some branches in the code, and the effect of
		// tail_shape is sufficiently subtle that there is no visible
		// difference in renders.
		const float TAIL_EPSILON = 0.0001f;
		if (std::abs(tail_shape - 1.0f) < TAIL_EPSILON)
			tail_shape += TAIL_EPSILON;

		// Precalculate normalization factor
		normalization_factor = normalization(roughness, tail_shape);
	}

	// Returns the normalization factor for the distribution function
	// of the BRDF.
	float normalization(float r, float t) const {
		const float r2 = r * r;
		const float top = (t - 1.0f) * (r2 - 1);
		const float bottom = M_PI * (1.0f - std::pow(r2, 1.0f - t));
		return top / bottom;
	}

	// Returns the cosine of the half-angle that should be sampled, given
	// a random variable in [0,1]
	float half_theta_sample(float u) const {
		const float roughness2 = roughness * roughness;

		// Calculate top half of equation
		const float top = 1.0f - std::pow((std::pow(roughness2, 1.0f - tail_shape) * (1.0f - u)) + u, 1.0f / (1.0f-tail_shape));

		// Calculate bottom half of equation
		const float bottom = (1.0f - roughness2);

		return std::sqrt(top / bottom);
	}


public:
	GTRClosure() {
		validate();
	}
	GTRClosure(Color col, float roughness, float tail_shape, float fresnel): col {col}, roughness {roughness}, tail_shape {tail_shape}, fresnel {fresnel} {
		validate();
	}


	virtual bool is_delta() const override {
		return roughness == 0.0f;
	}


	virtual void sample(const Vec3 &in, const DifferentialGeometry &geo, const float &si, const float &sj,
	                    Vec3 *out, Color *filter, float *pdf_) const override {
		// Get normalized surface normal
		Vec3 nn = geo.n.normalized();

		// If back-facing, flip normal
		if (dot(nn, in) > 0.0f) {
			nn *= -1.0f;
		}

		// Generate a random ray direction in the hemisphere
		// of the surface.
		const float theta_cos = half_theta_sample(si);
		const float theta_sin = std::sqrt(1.0f - (theta_cos * theta_cos));
		const float angle = sj * M_PI * 2;
		Vec3 half_dir {std::cos(angle) * theta_sin, std::sin(angle) * theta_sin, theta_cos};
		half_dir = zup_to_vec(half_dir, nn).normalized();

		*out = in - (half_dir * 2 * dot(in, half_dir));
		*pdf_ = pdf(in, *out, geo);
		*filter = col * evaluate(in, *out, geo);
	}


	Color evaluate(const Vec3 &in, const Vec3 &out, const DifferentialGeometry &geo) const override {
		// Calculate needed vectors, normalized
		Vec3 nn = geo.n.normalized();  // SUrface normal
		const Vec3 aa = in.normalized() * -1.0f;  // Vector pointing to where "in" came from
		const Vec3 bb = out.normalized(); // Out
		const Vec3 hh = ((aa + bb) * 0.5f).normalized(); // Half-way between aa and bb

		// If back-facing, flip normal
		if (dot(nn, hh) < 0.0f)
			nn *= -1.0f;

		// Calculate needed dot products
		const float na = clamp(dot(nn, aa), -1.0f, 1.0f);
		const float nb = clamp(dot(nn, bb), -1.0f, 1.0f);
		const float ha = clamp(dot(hh, aa), -1.0f, 1.0f);
		const float hb = clamp(dot(hh, bb), -1.0f, 1.0f);
		const float nh = clamp(dot(nn, hh), -1.0f, 1.0f);

		// Other useful numbers
		const float roughness2 = roughness * roughness;

		// Components of the bsdf, to be filled in below
		float D = 1.0f;
		float G1 = 1.0f;
		float G2 = 1.0f;
		float F = 1.0f;

		if (roughness != 0.0f) {
			// Calculate D - Distribution
			if (nh > 0.0f) {
				const float nh2 = nh * nh;
				D = normalization_factor / std::pow(1 + ((roughness2 - 1) * nh2), tail_shape);
			}

			// Calculate G1 - Geometric microfacet shadowing
			const float na2 = na * na;
			const float tan_na = std::sqrt((1.0f - na2) / na2);
			const float g1_pos_char = (ha*na) > 0.0f ? 1.0f : 0.0f;
			const float g1_a = roughness2 * tan_na;
			const float g1_b = (std::sqrt(1.0f + g1_a*g1_a) - 1.0f) * 0.5f;
			G1 = g1_pos_char / (1.0f + g1_b);

			// Calculate G2 - Geometric microfacet shadowing
			const float nb2 = nb * nb;
			const float tan_nb = std::sqrt((1.0f - nb2) / nb2);
			const float g2_pos_char = (hb*nb) > 0.0f ? 1.0f : 0.0f;
			const float g2_a = roughness2 * tan_nb;
			const float g2_b = (std::sqrt(1.0f + g2_a*g2_a) - 1.0f) * 0.5f;
			G2 = g2_pos_char / (1.0f + g2_b);
		}

		// Calculate F - Fresnel
		F = fresnel + ((1.0f-fresnel) * std::pow((1.0f - hb), 5.0f));

		// Final result
		return col * (D * F * G1 * G2);
	}


	float pdf(const Vec3& in, const Vec3& out, const DifferentialGeometry &geo) const override {
		// Calculate needed vectors, normalized
		Vec3 nn = geo.n.normalized();  // SUrface normal
		const Vec3 aa = in.normalized() * -1.0f;  // Vector pointing to where "in" came from
		const Vec3 bb = out.normalized(); // Out
		const Vec3 hh = ((aa + bb) * 0.5f).normalized(); // Half-way between aa and bb

		// If back-facing, flip normal
		if (dot(nn, hh) < 0.0f)
			nn *= -1.0f;

		// Calculate needed dot products
		const float nh = clamp(dot(nn, hh), -1.0f, 1.0f);

		// Other useful numbers
		const float roughness2 = roughness * roughness;

		// Calculate D - Distribution
		float D = 0.0f;
		if (nh > 0.0f) {
			const float nh2 = nh * nh;
			D = normalization_factor / std::pow(1 + ((roughness2 - 1) * nh2), tail_shape);
		}

		return D;
	}


	void propagate_differentials(const float t, const WorldRay& in, const DifferentialGeometry &geo, WorldRay* out) const override {
		Vec3 nn = geo.n.normalized();
		const Vec3 hh = ((in.d * -1.0f).normalized() + out->d.normalized()).normalized();

		// Project origin differentials
		out->odx = transfer_ray_origin_differential(t, nn, in.d, in.odx, in.ddx);
		out->ody = transfer_ray_origin_differential(t, nn, in.d, in.ody, in.ddy);

		// Calculate du and dv
		const auto duvdx = calc_uv_differentials(out->odx, geo.dpdu, geo.dpdv);
		const auto duvdy = calc_uv_differentials(out->ody, geo.dpdu, geo.dpdv);

		// Calculate normal differentials for this ray
		Vec3 dndx = (geo.dndu * duvdx.first) + (geo.dndv * duvdx.second);
		Vec3 dndy = (geo.dndu * duvdy.first) + (geo.dndv * duvdy.second);

		// Make sure nn and hh are facing the same direction
		if (dot(nn, hh) < 0.0f) {
			nn *= -1.0f;
			dndx *= -1.0f;
			dndy *= -1.0f;
		}

		// Transform normal differentials to be relative to the half-vector
		const Vec3 axis = cross(nn, hh).normalized();
		const float angle = std::acos(clamp(dot(nn, hh), 0.0f, 1.0f));
		const Transform xform = make_axis_angle_transform(axis, angle);
		const Vec3 dhdx = xform.dir_to(dndx);
		const Vec3 dhdy = xform.dir_to(dndy);

		// Reflect differential rays
		out->ddx = reflect_ray_direction_differential(hh, dhdx, in.d, in.ddx);
		out->ddy = reflect_ray_direction_differential(hh, dhdy, in.d, in.ddy);

		// Clamp ray direction differentials
		clamp_dd(out);
	}
};


#endif // SURFACE_CLOSURE_HPP