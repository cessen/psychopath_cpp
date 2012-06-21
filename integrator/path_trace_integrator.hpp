/*
 * This file and integrator.cpp define a Integrator class, which decides where
 * to shoot rays and how to combine their results into a final image or images.
 */
#ifndef PATH_TRACE_INTEGRATOR_HPP
#define PATH_TRACE_INTEGRATOR_HPP

#include "integrator.hpp"

#include "numtype.h"
#include "functor.hpp"

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
	Tracer *tracer;
	Film<Color> *image;
	int spp;
	int path_length;
	int thread_count;
	Functor *callback;

	/**
	 * @brief Constructor.
	 *
	 * @param[in] scene_ A pointer to the scene to render.  Should be fully
	 *                   finalized for rendering.
	 * @param[in] tracer_ A Tracer instance to use for the ray tracing.  It
	 *                    should already be fully initialized.
	 * @param[out] image_ The image to render to.  Should be already
	 *                    initialized with 3 channels, for rgb.
	 * @param spp_ The number of samples to take per pixel for integration.
	 */
	PathTraceIntegrator(Scene *scene_, Tracer *tracer_, Film<Color> *image_, int spp_, int thread_count_=1, Functor *callback_=NULL) {
		scene = scene_;
		tracer = tracer_;
		image = image_;
		spp = spp_;
		thread_count = thread_count_;
		path_length = 3;
		callback = callback_;
	}

	//virtual ~PathTraceIntegrator() {
	//	delete accum;
	//}

	/**
	 * @brief Begins integration.
	 */
	virtual void integrate();

};


/*
 * A path tracing path.
 * Stores state of a path in progress.
 */
struct PTPath {
	Intersection inter;
	Color col; // Color of the sample collected so far
	Color fcol; // Accumulated filter color from light path
	Color lcol; // Temporary storage for incoming light color

	bool done;
};

#endif // PATH_TRACE_INTEGRATOR_H

