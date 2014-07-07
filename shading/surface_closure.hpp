#ifndef SURFACE_CLOSURE_HPP
#define SURFACE_CLOSURE_HPP

#include <cmath>

#include "numtype.h"

#include "vector.hpp"
#include "color.hpp"
#include "ray.hpp"

class SurfaceClosure
{
public:
	// Virtual destructor, and don't delete default copy/move constructors
	SurfaceClosure() = default;
	virtual ~SurfaceClosure() = default;
	SurfaceClosure(const SurfaceClosure&) = default;
	SurfaceClosure(SurfaceClosure&&) = default;
	SurfaceClosure& operator=(const SurfaceClosure&) = default;
	SurfaceClosure& operator=(SurfaceClosure&&) = default;

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
		*pdf = dir.z * 2;
		if (*pdf < 0.001)
			*pdf = 0.001;
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
		const Vec3 dn = in.d.normalized();

		Vec3 x, y;
		coordinate_system_from_vec3(out->d, &x, &y);
		x.normalize();
		y.normalize();

		out->odx = transfer_ray_origin_differential(t, nn, dn, in.odx, in.ddx);
		out->ody = transfer_ray_origin_differential(t, nn, dn, in.ody, in.ddy);
		out->ddx = x * 0.15f / len;
		out->ddy = y * 0.15f / len;
	}


	float pdf(const Vec3& in, const Vec3& out, const DifferentialGeometry &geo) const override {
		Vec3 nn = geo.n.normalized();
		const Vec3 v = out.normalized();

		if (dot(nn, in.normalized()) > 0.0f)
			nn *= -1.0f;

		return std::max(dot(nn, v)*2, 0.0f);
	}
};




/**
 * The GTR microfacet BRDF from the Disney Principled BRDF paper.
 */
class GTRClosure final: public SurfaceClosure
{
private:
	Color col {1.0f};
	float roughness {0.2f};
	float tail_strength {2.0f};
	float fresnel {0.2f};
	float normalization_factor = normalization(roughness*roughness, tail_strength);

	// Returns the normalization factor for the distribution function
	// of the BRDF.
	float normalization(float r, float t) const {
		const float r2 = r * r;
		const float top = (t - 1.0f) * (r2 - 1);
		const float bottom = M_PI * (1.0f - std::pow(r2, 1.0f - t));
		return top / bottom;
	}

	// Calculate D - Distribution
	float d(const Vec3 n, const Vec3 m) const {
		const float roughness2 = roughness * roughness;
		const float roughness4 = roughness2 * roughness2;

		const float theta_cos = dot(n, m);
		const float theta_cos2 = theta_cos * theta_cos;
		const float theta_sin2 = 1.0f - theta_cos2;

		float d1 = 0.0f;
		if (theta_cos > 0.0f) {
			d1 = normalization_factor / std::pow(((roughness4*theta_cos2) + theta_sin2), tail_strength);
		}

		return d1;
	}

	// Calculate G - Geometric microfacet shadowing
	float g(const Vec3 n, const Vec3 m, const Vec3 v) const {
		//const float g_roughness = (1.0 + roughness) * 0.5f;
		const float g_roughness = roughness;
		const float g_roughness2 = g_roughness * g_roughness;

		const float cos_hv = dot(m, v);
		const float cos_nv = dot(n, v);
		const float cos_nv2 = cos_nv * cos_nv;
		const float tan_nv2 = (1.0f - cos_nv2) / cos_nv2;
		const float tan_nv = std::sqrt(tan_nv2);

		const float pos_char = (cos_hv*cos_nv) > 0.0f ? 1.0f : 0.0f;

		const float a = g_roughness2 * tan_nv;
		const float f1 = (std::sqrt(1.0f + a*a) - 1.0f) * 0.5f;
		return pos_char / (1.0f + f1);
	}

	// Returns the cosine of the half-angle that should be sampled, given
	// a random variable in [0,1]
	float half_theta_sample(float u) const {
		const float roughness2 = roughness * roughness;
		const float roughness4 = roughness2 * roughness2;

		const float f1 = std::pow((std::pow(roughness4, 1.0f - tail_strength) * (1.0f - u)) + u, 1.0f / (1.0f-tail_strength));
		const float f2 = std::sqrt((1.0f - f1) / (1.0f-roughness4));

		return f2;
	}

public:
	GTRClosure() = default;
	GTRClosure(Color col, float roughness, float tail_strength, float fresnel):
		col {col}, roughness {roughness}, tail_strength {tail_strength}, fresnel {fresnel} {
		// Clamp values to valid ranges
		roughness = std::max(0.0f, std::min(1.0f, roughness));
		tail_strength = std::max(1.0f, tail_strength);

		normalization_factor = normalization(roughness*roughness, tail_strength);
	}


	virtual bool is_delta() const override {
		return roughness <= 0.0f;
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
		Vec3 nn = geo.n.normalized();
		const Vec3 v1 = out.normalized();
		const Vec3 v2 = in.normalized() * -1.0f;

		// If back-facing, flip normal
		if (dot(nn, v2) < 0.0f)
			nn *= -1.0f;

		const Vec3 half = ((v1 + v2) * 0.5f).normalized();

		// Calculate D - Distribution
		const float D = d(nn, half);

		// Calculate F - Fresnel
		const float theta_cos = std::max(0.0f, dot(half, out));
		const float F = fresnel + ((1.0f-fresnel) * std::pow((1.0f - theta_cos), 5.0f));

		// Calculate G - Geometric microfacet shadowing
		const float G1 = g(nn, half, v1);
		const float G2 = g(nn, half, v2);


		const float cos_hv = dot(half, v1);
		const float cos_nv = dot(nn, v1);
		return col * (D * F * G1 * G2);
	}


	float pdf(const Vec3& in, const Vec3& out, const DifferentialGeometry &geo) const override {
		Vec3 nn = geo.n.normalized();
		const Vec3 v1 = out.normalized();
		const Vec3 v2 = in.normalized() * -1.0f;

		// If back-facing, flip normal
		if (dot(nn, v2) < 0.0f)
			nn *= -1.0f;

		const Vec3 half = ((v1 + v2) * 0.5f).normalized();
		const float roughness2 = roughness * roughness;
		const float roughness4 = roughness2 * roughness2;
		const float theta_cos = std::max(dot(nn, half), 0.0f);
		const float theta_cos2 = theta_cos * theta_cos;
		const float theta_sin2 = 1.0f - theta_cos2;

		// Calculate d - Distribution
		// roughness4 is used here for more intuitive parameter behavior
		const float d = normalization_factor / std::pow(((roughness4*theta_cos2) + theta_sin2), tail_strength);

		return d;
	}


	void propagate_differentials(const float t, const WorldRay& in, const DifferentialGeometry &geo, WorldRay* out) const override {
		const float len = out->d.length();
		const Vec3 nn = geo.n.normalized();
		const Vec3 dn = in.d.normalized();

		Vec3 x, y;
		coordinate_system_from_vec3(out->d, &x, &y);
		x.normalize();
		y.normalize();

		out->odx = transfer_ray_origin_differential(t, nn, dn, in.odx, in.ddx);
		out->ody = transfer_ray_origin_differential(t, nn, dn, in.ody, in.ddy);
		out->ddx = x * 0.01f / len;
		out->ddy = y * 0.01f / len;
	}
};


#endif // SURFACE_CLOSURE_HPP