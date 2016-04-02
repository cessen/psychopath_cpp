#ifndef SURFACE_CLOSURE_HPP
#define SURFACE_CLOSURE_HPP

#include <cmath>

#include "numtype.h"
#include "utils.hpp"
#include "monte_carlo.hpp"
#include "halton.hpp"

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
class SurfaceClosure {
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
	 * @param [in] wavelength  The wavelength of light to sample at.
	 *
	 * @param [out] out    The generated outgoing light direction.
	 * @param [out] filter The generated color filter.
	 * @param [out] pdf    The pdf value of the outgoing ray.
	 */
	virtual void sample(const Vec3 &in, const DifferentialGeometry &geo, const float &si, const float &sj, const float wavelength,
	                    Vec3 *out, SpectralSample *filter, float *pdf) const = 0;


	/**
	 * Evaluates the closure for the given incoming and outgoing rays.
	 *
	 * @param [in] in  The incoming light direction.
	 * @param [in] out The outgoing light direction.
	 * @param [in] geo The differential geometry of the reflecting/transmitting surface point.
	 * @param [in] wavelength The wavelength of light to evaluate for.
	 *
	 * @return The resulting filter color.
	 */
	virtual SpectralSample evaluate(const Vec3 &in, const Vec3 &out, const DifferentialGeometry &geo, const float wavelength) const = 0;


	/**
	 * Estimates how much light propigates from the given circular solid angle
	 * (out, cos_theta) to the given direction "in".
	 *
	 * This function does not need to be 100% accurate, but it does need to
	 * never be zero where the exact solution wouldn't be zero.  It is used
	 * for importance sampling.
	 *
	 * @param [in] in  The incoming light direction.
	 * @param [in] out The direction of the outgoing light solid angle.
	 * @param [in] cos_theta The cosine of the radius of the solid angle.
	 * @param [in] nor The surface normal at the surface point.
	 * @param [in] wavelength The wavelength of light to evaluate for.
	 */
	virtual float estimate_eval_over_solid_angle(const Vec3 &in, const Vec3 &out, const float cos_theta, const Vec3 nor, const float wavelength) const = 0;


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
	virtual float sample_pdf(const Vec3& in, const Vec3& out, const DifferentialGeometry &geo) const = 0;
};


/**
 * Utility function that calculates the fresnel reflection factor of a given
 * incoming ray against a surface with the given ior outside/inside ratio.
 *
 * @param ior_ratio The ratio of the outside material ior (probably 1.0 for
 *                  air) over the inside ior.
 * @param c The cosine of the angle between the incoming light and the
 *          surface's normal.  Probably calculated e.g. with a normalized
 *          dot product.
 */
static inline float dielectric_fresnel(float ior_ratio, float c) {
	const float g = std::sqrt(ior_ratio - 1.0f + (c * c));

	const float f1 = g - c;
	const float f2 = g + c;
	const float f3 = (f1 * f1) / (f2 * f2);

	const float f4 = (c * f2) - 1.0f;
	const float f5 = (c * f1) + 1.0f;
	const float f6 = 1.0f + ((f4 * f4) / (f5 * f5));

	return 0.5f * f3 * f6;
}


/**
 * Schlick's approximation of the fresnel reflection factor.
 *
 * Same interface as dielectric_fresnel(), above.
 */
static inline float schlick_fresnel(float ior_ratio, float c) {
	const float f1 = (1.0f - ior_ratio) / (1.0f + ior_ratio);
	const float f2 = f1 * f1;
	const float c1 = (1.0f - c);
	const float c2 = c1 * c1;
	return f2 + ((1.0f-f2) * c1 * c2 * c2);
}


/**
 * Utility function that calculates the fresnel reflection factor of a given
 * incoming ray against a surface with the given normal-reflectance factor.
 *
 * @param frensel_fac The ratio of light reflected back if the ray were to
 *                    hit the surface head-on (perpendicular to the surface).
 * @param c The cosine of the angle between the incoming light and the
 *          surface's normal.  Probably calculated e.g. with a normalized
 *          dot product.
 */
static inline float dielectric_fresnel_from_fac(float fresnel_fac, float c) {
	const float tmp1 = std::sqrt(fresnel_fac) - 1.0f;

	// Protect against divide by zero.
	if (std::abs(tmp1) < 0.000001)
		return 1.0f;

	// Find the ior ratio
	const float tmp2 = (-2.0f / tmp1) - 1.0f;
	const float ior_ratio = tmp2 * tmp2;

	// Calculate fresnel factor
	return dielectric_fresnel(ior_ratio, c);
}



/**
 * Schlick's approximation version of dielectric_fresnel_from_fac() above.
 */
static inline float schlick_fresnel_from_fac(float frensel_fac, float c) {
	const float c1 = (1.0f - c);
	const float c2 = c1 * c1;
	return frensel_fac + ((1.0f-frensel_fac) * c1 * c2 * c2);
}


/**
 * Emit closure.
 *
 * NOTE: this needs to be handled specially by the integrator!  It does not
 * behave like a standard closure!
 */
class EmitClosure final: public SurfaceClosure {
	Color col {1.0f};

public:
	EmitClosure() = default;
	EmitClosure(Color col): col {col} {}


	virtual bool is_delta() const override {
		return false;
	}


	virtual void sample(const Vec3 &in, const DifferentialGeometry &geo, const float &si, const float &sj, const float wavelength,
	                    Vec3 *out, SpectralSample *filter, float *pdf) const override {
		*pdf = 1.0f;
		*out = Vec3(1.0f);
		*filter = SpectralSample {wavelength, 0.0f};
	}


	SpectralSample evaluate(const Vec3 &in, const Vec3 &out, const DifferentialGeometry &geo, const float wavelength) const override {
		return SpectralSample {wavelength, 0.0f};
	}

	float estimate_eval_over_solid_angle(const Vec3 &in, const Vec3 &out, const float cos_theta, const Vec3 nor, const float wavelength) const override {
		return 1.0f;
	}


	void propagate_differentials(const float t, const WorldRay& in, const DifferentialGeometry &geo, WorldRay* out) const override {
		// Irrelivant
	}


	float sample_pdf(const Vec3& in, const Vec3& out, const DifferentialGeometry &geo) const override {
		return 1.0f;
	}

	SpectralSample emitted_color(const float wavelength) const {
		return Color_to_SpectralSample(col, wavelength);
	}
};




class LambertClosure final: public SurfaceClosure {
	Color col {1.0f};

public:
	LambertClosure() = default;
	LambertClosure(Color col): col {col} {}


	virtual bool is_delta() const override {
		return false;
	}


	virtual void sample(const Vec3 &in, const DifferentialGeometry &geo, const float &si, const float &sj, const float wavelength,
	                    Vec3 *out, SpectralSample *filter, float *pdf) const override {
		// Get normalized surface normal
		Vec3 nn = geo.n.normalized();

		// If back-facing, flip normal
		if (dot(nn, in) > 0.0f) {
			nn *= -1.0f;
		}

		// Generate a random ray direction in the hemisphere
		// of the surface.
		const Vec3 dir = cosine_sample_hemisphere(si, sj);
		*pdf = dir.z * (float)(INV_PI);
		*out = zup_to_vec(dir, nn);
		*filter = evaluate(in, *out, geo, wavelength);
	}


	SpectralSample evaluate(const Vec3 &in, const Vec3 &out, const DifferentialGeometry &geo, const float wavelength) const override {
		Vec3 nn = geo.n.normalized();
		const Vec3 v = out.normalized();

		if (dot(nn, in) > 0.0f)
			nn *= -1.0f;

		const float fac = std::max(dot(nn, v), 0.0f) * (float)(INV_PI);
		return Color_to_SpectralSample(col * fac, wavelength);
	}

	float estimate_eval_over_solid_angle(const Vec3 &in, const Vec3 &out, const float cos_theta, const Vec3 nor, const float wavelength) const override {
		assert(cos_theta >= -1.0f && cos_theta <= 1.0f);

// 		Vec3 nn = nor.normalized();
// 		const Vec3 v = out.normalized();

// 		if (dot(nn, in) > 0.0f)
// 			nn *= -1.0f;

// 		const float cos_nv = dot(nn, v);
// 		const float blend = clamp(cos_theta, 0.0f, 1.0f);
// 		float fac = lerp(blend, (float)(M_PI), std::max(0.0f, cos_nv)) * std::min(1.0f - cos_theta, 1.0f);

// 		return fac;

		if (cos_theta < 0.0f) {
			return 1.0f;
		} else {
			Vec3 nn = nor.normalized();
			const Vec3 v = out.normalized();

			if (dot(nn, in) > 0.0f)
				nn *= -1.0f;

			const float cos_nv = dot(nn, v);

			return sphere_lambert(cos_nv, cos_theta);
		}
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


	float sample_pdf(const Vec3& in, const Vec3& out, const DifferentialGeometry &geo) const override {
		Vec3 nn = geo.n.normalized();
		const Vec3 v = out.normalized();

		if (dot(nn, in.normalized()) > 0.0f)
			nn *= -1.0f;

		return std::max(dot(nn, v) * (float)(INV_PI), 0.0f);
	}
};




/**
 * The GTR microfacet BRDF from the Disney Principled BRDF paper.
 */
class GTRClosure final: public SurfaceClosure {
private:
	Color col {1.0f};
	float roughness {0.05f};
	float tail_shape {2.0f};
	float fresnel {1.0f};
	float normalization_factor = normalization(roughness, tail_shape);



	// Makes sure values are in a valid range
	void validate() {
		// Clamp values to valid ranges
		roughness = std::max(0.0f, std::min(0.9999f, roughness));
		tail_shape = std::max(0.0001f, tail_shape);

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
			tail_shape = 1.0f + TAIL_EPSILON;

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

	float dist(float nh, float rough) const {
		// Other useful numbers
		const float roughness2 = rough * rough;

		// Calculate D - Distribution
		float D = 0.0f;
		if (nh > 0.0f) {
			const float nh2 = nh * nh;
			D = normalization(rough, tail_shape) / std::pow(1 + ((roughness2 - 1) * nh2), tail_shape);
		}

		return D;
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


	virtual void sample(const Vec3 &in, const DifferentialGeometry &geo, const float &si, const float &sj, const float wavelength,
	                    Vec3 *out, SpectralSample *filter, float *pdf) const override {
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
		*pdf = sample_pdf(in, *out, geo);
		*filter = evaluate(in, *out, geo, wavelength);

	}


	SpectralSample evaluate(const Vec3 &in, const Vec3 &out, const DifferentialGeometry &geo, const float wavelength) const override {
		// Calculate needed vectors, normalized
		Vec3 nn = geo.n.normalized();  // SUrface normal
		const Vec3 aa = in.normalized() * -1.0f;  // Vector pointing to where "in" came from
		const Vec3 bb = out.normalized(); // Out
		const Vec3 hh = (aa + bb).normalized(); // Half-way between aa and bb

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

		// Calculate F - Fresnel
		SpectralSample col_f = Color_to_SpectralSample(col, wavelength);
		for (int i = 0; i < SPECTRAL_COUNT; ++i) {
			col_f.e[i] = lerp(1.0f - fresnel, schlick_fresnel_from_fac(col_f.e[i], hb), col_f.e[i]);
		}

		// Calculate everything else
		if (roughness == 0.0f) {
			// If sharp mirror, just return col * fresnel factor
			return col_f;
		} else {
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

		// Final result
		return col_f * (D * G1 * G2) * (float)(INV_PI);
	}


	float estimate_eval_over_solid_angle(const Vec3 &in, const Vec3 &cent, const float cos_theta, const Vec3 nor, const float wavelength) const override {
		// TODO: all of the stuff in this function is horribly hacky.
		// Find a proper way to approximate the light contribution from a
		// solid angle.
		assert(cos_theta >= -1.0f && cos_theta <= 1.0f);

		Vec3 nn = nor.normalized();
		const Vec3 aa = in.normalized() * -1.0f;  // Vector pointing to where "in" came from
		const Vec3 bb = cent.normalized(); // Out

		if (dot(nn, in) > 0.0f)
			nn *= -1.0f;

		// Brute-force method
		//float fac = 0.0f;
		//constexpr int N = 256;
		//for (int i = 0; i < N; ++i) {
		//    const float uu = Halton::sample(0, i);
		//    const float vv = Halton::sample(1, i);
		//    Vec3 samp = uniform_sample_cone(uu, vv, cos_theta);
		//    samp = zup_to_vec(samp, bb).normalized();
		//    if (dot(nn, samp) > 0.0f) {
		//        const Vec3 hh = (aa+samp).normalized();
		//        fac += dist(dot(nn, hh), roughness);
		//    }
		//}
		//fac /= N * N;

		// Approximate method
		const float theta = std::acos(cos_theta);
		const Vec3 hh = (aa + bb).normalized();
		const float nh = clamp(dot(nn, hh), -1.0f, 1.0f);
		const float fac = dist(nh, std::min(1.0f, std::sqrt(roughness) + (2.0f*theta/(float)(M_PI))));

		return fac * std::min(1.0f - cos_theta, 1.0f) * (float)(INV_PI);
	}


	float sample_pdf(const Vec3& in, const Vec3& out, const DifferentialGeometry &geo) const override {
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

		return dist(nh, roughness) * (float)(INV_PI);
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