#ifndef SURFACE_SHADER_HPP
#define SURFACE_SHADER_HPP

#include "numtype.hpp"

#include "intersection.hpp"
#include "surface_closure.hpp"

class SurfaceShader {
	virtual ~SurfaceShader() {}
	
	/**
	 * @brief Calculates the SurfaceClosure for the given intersection.
	 */
	virtual SurfaceClosure *shade(const Intersection &inter) = 0;
};

#endif // SURFACE_SHADER_HPP