/*
 * This file and integrator.cpp define a Integrator class, which decides where
 * to shoot rays and how to combine their results into a final image or images.
 */
#ifndef PATH_TRACE_INTEGRATOR_HPP
#define PATH_TRACE_INTEGRATOR_HPP

#include <functional>

#include "numtype.h"

#include "integrator.hpp"
#include "film.hpp"
#include "scene.hpp"
#include "tracer.hpp"
#include "color.hpp"

/**
 * @brief An integrator for the rendering equation.
 *
 * The Integrator's job is to solve the rendering equation, using the Tracer
 * for ray intersection testing and the shading system for shading.
 *
 * It will implement path tracing with next event estimation.  But it
 * could instead, for example, implement Whitted style ray tracing, or
 * bidirectional path tracing, or metroplis light transport, etc.
 * Although markov chain algorithms may play poorly with the Tracer, which is
 * designed to trace rays in bulk.
 */
class PathTraceIntegrator: Integrator
{
public:
	Scene *scene;
	std::vector<Tracer> tracers;
	Film<Color> *image;
	int spp;
	uint seed;
	int path_length;
	int thread_count;
	std::function<void()> callback;

	/**
	 * @brief Constructor.
	 *
	 * @param[in] scene_ A pointer to the scene to render.  Should be fully
	 *                   finalized for rendering.
	 * @param[in] tracers_ A Tracer instance to use for the ray tracing.  It
	 *                    should already be fully initialized.
	 * @param[out] image_ The image to render to.  Should be already
	 *                    initialized with 3 channels, for rgb.
	 * @param spp_ The number of samples to take per pixel for integration.
	 */
	PathTraceIntegrator(Scene *scene_, std::vector<Tracer> tracers_, Film<Color> *image_, int spp_, uint seed_, int thread_count_=1, std::function<void()> callback_ = std::function<void()>()) {
		scene = scene_;
		tracers = tracers_;
		image = image_;
		spp = spp_;
		seed = seed_;
		thread_count = thread_count_;
		path_length = 3;
		callback = callback_;
	}

	/**
	 * @brief Begins integration.
	 */
	virtual void integrate();

};

#endif // PATH_TRACE_INTEGRATOR_H

