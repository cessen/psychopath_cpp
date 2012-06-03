/*
 * This file and renderer.cpp define a Renderer class, which serves as
 * as the API for setting up, running, and controlling a render.
 */
#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "numtype.h"
#include <string>
#include <vector>

/*
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
	string::string output_path;

public:
	// Starts a render with the given number of threads.
	void render(int thread_count);
}



#endif // RENDERER_H

