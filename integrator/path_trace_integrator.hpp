/*
 * This file and integrator.cpp define a Integrator class, which decides where
 * to shoot rays and how to combine their results into a final image or images.
 */
#ifndef PATH_TRACE_INTEGRATOR_HPP
#define PATH_TRACE_INTEGRATOR_HPP

#include <iostream>
#include <iomanip>

#include <functional>
#include <mutex>

#include "numtype.h"

#include "spinlock.hpp"

#include "integrator.hpp"
#include "film.hpp"
#include "scene.hpp"
#include "tracer.hpp"
#include "color.hpp"

#include "ring_buffer_concurrent.hpp"

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
	struct PixelBlock {
		int x, y;
		int w, h;
	};

	/*
	 * A path tracing path state.
	 * Stores state of a path in progress.
	 */
	struct PTState {
		const float *samples = nullptr;
		float time;
		int step = 0;
		short pix_x, pix_y;  // Pixel coordinates of the path
		Intersection inter = Intersection();
		Color col {0.0f}; // Color of the sample collected so far
		Color fcol {1.0f}; // Accumulated filter color from light path
		Color lcol {0.0f}; // Temporary storage for incoming light color

		bool done {false};
	};

	void init_path(PTState* pstate, const float* samps, short x, short y);
	WorldRay next_ray_for_path(const WorldRay& prev_ray, PTState* pstate);
	void update_path(PTState* pstate, const WorldRay& ray, const Intersection& inter);



	size_t total_items = 0;
	size_t completed_items = 0;
	SpinLock progress_lock;
	void print_progress() {
		std::cout << "\rRendering: " << std::fixed << std::setprecision(2) << (float(completed_items) / total_items) * 100 << "%" << std::flush;
	}



public:
	Scene *scene;
	Film<Color> *image;
	std::mutex image_mut;
	int spp;
	int spp_max;
	float image_variance_max;
	uint seed;
	int path_length;
	int thread_count;
	std::function<void()> callback;

	RingBufferConcurrent<PixelBlock> blocks; // Queue for pending blocks of pixels to be rendered

	/**
	 * @brief Constructor.
	 *
	 * @param[in] scene_ A pointer to the scene to render.  Should be fully
	 *                   finalized for rendering.
	 * @param[out] image_ The image to render to.  Should be already
	 *                    initialized with 3 channels, for rgb.
	 * @param spp_ The number of samples to take per pixel for integration.
	 */
	PathTraceIntegrator(Scene *scene_, Film<Color> *image_, int spp_, int spp_max_, float variance_max_, uint seed_, int thread_count_=1, std::function<void()> callback_ = std::function<void()>()) {
		scene = scene_;
		image = image_;
		spp = spp_;
		spp_max = spp_max_;
		image_variance_max = variance_max_;
		seed = seed_;
		thread_count = thread_count_;
		path_length = 3;
		callback = callback_;

		blocks.resize(thread_count_ * 2);
	}

	/**
	 * @brief Begins integration.
	 */
	virtual void integrate();

	/**
	 * Watches the block queue for blocks of pixels to render.
	 */
	void render_blocks();
};

#endif // PATH_TRACE_INTEGRATOR_H

