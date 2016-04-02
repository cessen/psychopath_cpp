/*
 * This file and renderer.cpp define a Renderer class, which serves as
 * as the API for setting up, running, and controlling a render.
 */
#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "numtype.h"
#include <memory>
#include <string>
#include <vector>
#include <OpenImageIO/imageio.h>

#include "scene.hpp"

/**
 * @brief Manages a render.
 *
 * The Renderer is responsible for doing the actual rendering.  It is given
 * 3d scene that has already been setup, and it dives in and tears it
 * to pieces.  The result is an image or images.
 *
 * The Renderer is responsible for knowing:
 * - Where to output the render result (e.g. to a file, or files, to
 *   another process...)
 * - What "passes" to output (light path expressions) and in what format.
 * - What resolution to render with.
 * - How to manage resources during rendering (number of threads to use, RAM
 *   usage limits, max grid size, bucket size, ray buffer size...)
 * - Render quality settings (number of samples, adaptive sampling settings,
 *   dicing rate, color clamping...).
 *
 * Essentially, anything that is not part of the scene description is entirely
 * the responsibility of the renderer.
 */
class Renderer {
private:
	uint res_x, res_y;
	uint subimage_x1, subimage_y1, subimage_x2, subimage_y2;
	uint spp;
	uint spp_max;
	float variance_max;
	uint seed;
	std::string output_path;

public:
	std::unique_ptr<Scene> scene;

	Renderer(Scene *scene, uint res_x, uint res_y, uint spp, uint spp_max, float variance_max, uint seed, std::string output_path):
		res_x {res_x},
	      res_y {res_y},
	      subimage_x1 {0}, subimage_y1 {0}, subimage_x2 {res_x}, subimage_y2 {res_y},
	      spp {spp},
	      spp_max {spp_max},
	      variance_max {variance_max},
	      seed {seed},
	      output_path {output_path},
	      scene {scene}
	{}

	void set_resolution(int res_x_, int res_y_) {
		res_x = res_x_;
		res_y = res_y_;
	}

	void set_subimage(int subimage_x1_, int subimage_y1_, int subimage_x2_, int subimage_y2_) {
		subimage_x1 = subimage_x1_;
		subimage_y1 = subimage_y1_;
		subimage_x2 = subimage_x2_;
		subimage_y2 = subimage_y2_;
	}

	void set_spp(int spp_) {
		spp = spp_;
	}

	void set_spp_max(int spp_max_) {
		spp_max = spp_max_;
	}

	void set_variance_max(float variance_max_) {
		variance_max = variance_max_;
	}

	// Starts a render with the given number of threads.
	bool render(int thread_count=1);
};



#endif // RENDERER_HPP

