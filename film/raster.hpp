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
template <class PIXFMT>
class Raster
{
public:
	uint16_t width, height; // Resolution of the image
	float min_x, min_y; // Minimum x/y coordinates of the image
	float max_x, max_y; // Maximum x/y coordinates of the image
	uint16_t channels; // Channels per pixel
	PIXFMT *pixels; // Pixel data

	/**
	 * @brief Constructor.
	 *
	 * Creates a new Raster buffer.  All pixel data is initialized to zero.
	 */
	Raster(int w, int h, int cc, float x1, float y1, float x2, float y2) {
		width = w;
		height = h;
		min_x = x1 < x2 ? x1 : x2;
		min_y = y1 < y2 ? y1 : y2;
		max_x = x1 > x2 ? x1 : x2;
		max_y = y1 > y2 ? y1 : y2;

		channels = cc;
		pixels = new PIXFMT[w*h*cc];
		for (int i=0; i < w*h*cc; i++)
			pixels[i] = 0;
	}

	~Raster() {
		delete [] pixels;
	}

	/**
	 * @brief Fetches a pointer to the requested pixel's data.
	 */
	PIXFMT *pixel(int x, int y) {
		assert(x >= 0 && x < width && y >= 0 && y < height);
		return &(pixels[(y*width + x)*channels]);
	}
};

#endif
