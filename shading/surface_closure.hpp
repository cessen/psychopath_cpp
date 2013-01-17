#ifndef SURFACE_CLOSURE_HPP
#define SURFACE_CLOSURE_HPP

#include "numtype.hpp"

#include "vector.hpp"
#include "color.hpp"
#include "ray.hpp"

class SurfaceClosure {
	virtual ~SurfaceClosure() {}
	
	/**
	 * @brief Returns whether the closure has a delta distribution
	 *        or not.
	 */
	virtual bool is_delta() const = 0;

	/**
	 * @brief Given an incoming ray and sample values, generates an outgoing ray and color filter.
	 *
	 * @param [in] in Incoming ray.
	 * @param [in] si A sample value.
	 * @param [in] sj A sample value.
	 * @param [in] sk A sample value.
	 *
	 * @param [out] out The generated outgoing ray.
	 * @param [out] filter The generated color filter.
	 * @param [out] pdf The pdf value of the outgoing ray.
	 */
	virtual void sample(const Ray &in, const float32 &si, const float32 &sj, const float32 &sk,
	                    Ray *out, Color *filter, float32 *pdf) = 0;
	
	/**
	 * @brief Evaluates the closure for the given incoming and outgoing rays.
	 *
	 * @param [in] in The incoming direction.
	 * @param [in] out The outgoing direction.
	 *
	 * @return The resulting filter color.
	 */
	virtual Color evaluate(const Vec3 &in, const Vec3 &out) = 0;
};

#endif // SURFACE_CLOSURE_HPP