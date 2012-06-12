/*
 * This file and integrator.cpp define a Integrator class, which decides where
 * to shoot rays and how to combine their results into a final image or images.
 */
#ifndef DIRECT_LIGHTING_INTEGRATOR_HPP
#define DIRECT_LIGHTING_INTEGRATOR_HPP

#include "integrator.hpp"

#include "numtype.h"

#include "raster.hpp"
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
class DirectLightingIntegrator: Integrator
{
public:
	Scene *scene;
	Tracer *tracer;
	Raster *image;
	Raster *accum;
	int spp;
	int thread_count;

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
	DirectLightingIntegrator(Scene *scene_, Tracer *tracer_, Raster *image_, int spp_, int thread_count_=1) {
		scene = scene_;
		tracer = tracer_;
		image = image_;
		spp = spp_;
		thread_count = thread_count_;

		accum = new Raster(image->width, image->height, 1, image->min_x, image->min_y, image->max_x, image->max_y);
	}

	virtual ~DirectLightingIntegrator() {
		delete accum;
	}

	/**
	 * @brief Begins integration.
	 */
	virtual void integrate();

};


/*
 * A direct lighting path.
 * Stores state of a path in progress.
 */
struct DLPath
{
	Intersection inter;
	Color col, lcol;
	bool done;
};

#endif // DIRECT_LIGHTING_INTEGRATOR_H

