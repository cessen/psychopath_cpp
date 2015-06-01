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
	 * @param inter The surface intersection data.
	 * @param max_closures The maximum number of closures that can be handled
	 *                     by the calling code.
	 * @param closures Pointer to an array of size max_closures of
	 *                 SurfaceClosureUnions, for the resulting surface
	 *                 closure(s) to be stored in.
	 * @param closure_pdfs Pointer to an array of size max_closures of floats
	 *                     for the returned closures' pdfs to be stored in.
	 *
	 * @returns The number of closures actually filled in.
	 */
	virtual int shade(const Intersection &inter, int max_closures,
	                  SurfaceClosureUnion *closures, float *closure_pdfs) const = 0;
};

#endif // SURFACE_SHADER_HPP