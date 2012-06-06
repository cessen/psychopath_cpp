#ifndef RASTER_HPP
#define RASTER_HPP

#include "numtype.h"

#include <assert.h>

/**
 * A lightweight raster image buffer.
 * Includes a mapping to 2d coordinates.
 * Pixels are stored in left-to-right, top-to-bottom order, with all the
 * channels of a pixel stored next to each other.
 */
class Raster
{
public:
	int width, height; // Resolution of the image
	float32 min_x, min_y; // Minimum x/y coordinates of the image
	float32 max_x, max_y; // Maximum x/y coordinates of the image
	int channels; // Channels per pixel
	float32 *pixels; // Pixel data

	/**
	 * @brief Constructor.
	 *
	 * Creates a new Raster buffer.  All pixel data is initialized to zero.
	 */
	Raster(int w, int h, int cc, float32 x1, float32 y1, float32 x2, float32 y2);
	~Raster();

	/**
	 * @brief Fetches a pointer to the requested pixel's data.
	 */
	float32 *pixel(int x, int y) {
		assert(x >= 0 && x < width && y >= 0 && y < height);
		return &(pixels[(y*width + x)*channels]);
	}
};

#endif
