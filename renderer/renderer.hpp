/*
 * This file and renderer.cpp define a Renderer class, which serves as
 * as the API for setting up, running, and controlling a render.
 */
#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "numtype.h"
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
class Renderer
{
private:
	uint res_x, res_y;
	uint spp;
	std::string output_path;

public:
	Scene *scene;

	Renderer(Scene *scene_, int res_x_, int res_y_, int spp_, std::string output_path_) {
		scene = scene_;
		res_x = res_x_;
		res_y = res_y_;
		spp = spp_;
		output_path = output_path_;
	}

	// Starts a render with the given number of threads.
	bool render(int thread_count=1);
};



#endif // RENDERER_HPP

