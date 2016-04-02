/*
 * This file defines the interface to Integrator classes, which decide where
 * to shoot rays and how to combine their results into a final image or images.
 */
#ifndef INTEGRATOR_HPP
#define INTEGRATOR_HPP

#include "numtype.h"

#include "raster.hpp"
#include "scene.hpp"
#include "tracer.hpp"

/**
 * @brief An integrator for the rendering equation.
 *
 * The Integrator's job is to solve the rendering equation, using the Tracer
 * for ray intersection testing and the shading system for shading.
 *
 * It can, for example, implement Whitted style ray tracing, or
 * bidirectional path tracing, or metroplis light transport, etc.
 * Although markov chain algorithms may play poorly with the Tracer, which is
 * designed to trace rays in bulk.
 */
class Integrator {
public:
	virtual ~Integrator() {}

	/**
	 * @brief Begins integration.
	 */
	virtual void integrate() = 0;

};

#endif // INTEGRATOR_H

