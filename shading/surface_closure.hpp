#ifndef SURFACE_CLOSURE_HPP
#define SURFACE_CLOSURE_HPP

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
		const Vec3 nn = geo.n.normalized();

		// If back-facing, darkness
		if (dot(nn, in) > 0.0f) {
			*out = Vec3(0.0f, 0.0f, 0.0f);
			*filter = Color(0.0f);
			*pdf = 1.0f;
		}

		// Generate a random ray direction in the hemisphere
		// of the surface.
		const Vec3 dir = cosine_sample_hemisphere(si, sj);
		*pdf = dir.z * 2;
		if (*pdf < 0.001)
			*pdf = 0.001;
		*filter = col * dir.z;
		*out = zup_to_vec(dir, nn);
	}


	Color evaluate(const Vec3 &in, const Vec3 &out, const DifferentialGeometry &geo) const override {
		const Vec3 nn = geo.n.normalized();
		const Vec3 v = out.normalized();

		if (dot(nn, in.normalized()) > 0.0f)
			return Color(0.0f);
		else
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
		const Vec3 nn = geo.n.normalized();
		const Vec3 v = out.normalized();

		if (dot(nn, in) > 0.0f)
			return 0.0f;
		else
			return std::max(dot(nn, v)*2, 0.0f);
	}
};


#endif // SURFACE_CLOSURE_HPP