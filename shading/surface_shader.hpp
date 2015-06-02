#ifndef SURFACE_SHADER_HPP
#define SURFACE_SHADER_HPP

#include "numtype.h"

#include "intersection.hpp"
#include "surface_closure.hpp"
#include "closure_union.hpp"

class SurfaceShader
{
public:
	virtual ~SurfaceShader() {}

	/**
	 * @brief Calculates the SurfaceClosure(s) and their pdfs for the given
	 * intersection.
	 *
	 * @param inter The surface intersection data.  This is an in/out parameter:
	 *              the geometry, transform, ray data, etc. is 'in' and the
	 *              closure data is 'out'.
	 *
	 * @returns True on success, false on failure.
	 */
	virtual bool shade(Intersection* inter) const = 0;
};


class EmitShader: public SurfaceShader
{
public:
	Color col;

	EmitShader(Color col): col {col} {}

	virtual bool shade(Intersection* inter) const override final {
		inter->surface_closure.init(EmitClosure(col));
		inter->closure_prob = 1.0f;
		return true;
	}
};


class LambertShader: public SurfaceShader
{
public:
	Color col;

	LambertShader(Color col): col {col} {}

	virtual bool shade(Intersection* inter) const override final {
		inter->surface_closure.init(LambertClosure(col));
		inter->closure_prob = 1.0f;
		return true;
	}
};


class GTRShader: public SurfaceShader
{
public:
	Color col;
	float roughness;
	float tail_shape;
	float fresnel;

	GTRShader(Color col, float roughness, float tail_shape, float fresnel): col {col}, roughness {roughness}, tail_shape {tail_shape}, fresnel {fresnel}
	{}

	virtual bool shade(Intersection* inter) const override final {
		inter->surface_closure.init(GTRClosure(col, roughness, tail_shape, fresnel));
		inter->closure_prob = 1.0f;
		return true;
	}
};

#endif // SURFACE_SHADER_HPP