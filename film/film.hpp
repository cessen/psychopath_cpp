#ifndef FILM_HPP
#define FILM_HPP

#include "numtype.h"

#include <algorithm>
#include <assert.h>

#include "morton.hpp"
#include "color.hpp"
#include "rng.hpp"
#include "blocked_array_disk_cache.hpp"

#define LBS 5

/**
 * @brief Pixel filter.
 *
 * Currently just implements a mitchel filter.
 */
class Filter
{
	float32 B, C;

public:
	Filter(float32 C_ = 0.5) {
		C = C_;
		B = 1.0 - (2*C);
	}

	float32 width() {
		return 2.0;
	}

	float32 samp_1d(float32 x) {
		x = std::abs(x);
		if (x > 2.f)
			return 0.f;
		if (x > 1.f)
			return ((-B - 6*C) * x*x*x + (6*B + 30*C) * x*x +
			        (-12*B - 48*C) * x + (8*B + 24*C)) * (1.f/6.f);
		else
			return ((12 - 9*B - 6*C) * x*x*x +
			        (-18 + 12*B + 6*C) * x*x +
			        (6 - 2*B)) * (1.f/6.f);
	}

	float32 samp_2d(float32 x, float32 y) {
		return samp_1d(x) * samp_1d(y);
	}
};


/**
 * Film that accumulates samples while rendering.
 */
template <class PIXFMT>
class Film
{
public:
	RNG rng;
	uint16 width, height; // Resolution of the image in pixels
	float32 min_x, min_y; // Minimum x/y coordinates of the image
	float32 max_x, max_y; // Maximum x/y coordinates of the image

	uint16 pad; // Amount of pixel padding added around the primary
	// resolution of the image.
	uint16 width_p, height_p; // Resolution in pixels after padding
	float32 min_x_p, min_y_p; // Minimum x/y coordinates of the image after padding
	float32 max_x_p, max_y_p; // Maximum x/y coordinates of the image after padding

	BlockedArrayDiskCache<PIXFMT, LBS> pixels; // Pixel data
	BlockedArrayDiskCache<float32, LBS> accum; // Accumulation buffer
	Filter filter;
	uint8 filter_width;

	/**
	 * @brief Constructor.
	 *
	 * Creates a new Film.  All pixel values are initialized to a zeroed state.
	 */
	Film(int w, int h, int padding, float32 x1, float32 y1, float32 x2, float32 y2) {
		// Store meta data
		pad = padding;
		width = w;
		height = h;
		width_p = w + (pad * 2);
		height_p = h + (pad * 2);
		min_x = std::min(x1, x2);
		min_y = std::min(y1, y2);
		max_x = std::max(x1, x2);
		max_y = std::max(y1, y2);
		const float32 wd = ((max_x - min_x) * pad) / width;
		const float32 hd = ((max_y - min_y) * pad) / height;
		min_x_p = min_x - wd;
		max_x_p = max_x + wd;
		min_y_p = min_y - hd;
		max_y_p = max_y + hd;

		// Allocate pixel and accum data
		pixels.init(width_p, height_p);
		accum.init(width_p, height_p);

		// Zero out pixels and accum
		std::cout << "Clearing out\n";
		uint32 u = 0;
		uint32 v = 0;
		for (uint32 i = 0; u < width_p || v < height_p; i++) {
			Morton::d2xy(i, &u, &v);
			if (u < width_p && v < height_p) {
				pixels(u,v) = PIXFMT(0);
				accum(u,v) = 0.0;
			}
		}

		// Set up the pixel filter
		filter = Filter();
		filter_width = std::ceil(filter.width());

		// Set up RNG
		rng = RNG(7373546);
	}

	//~Film() {
	//	delete [] pixels;
	//	delete [] accum;
	//}

	/**
	 * @brief Adds a sample to the film.
	 */
	void add_sample(PIXFMT samp, float32 x, float32 y) {
		// Convert to pixel-space coordinates
		x = (x * width_p) - 0.5;
		y = (y * height_p) - 0.5;

		for (int j=-filter_width; j <= filter_width; j++) {
			for (int k=-filter_width; k <= filter_width; k++) {
				int a = x + j;
				int b = y + k;

				if (a < 0 || a >= width_p || b < 0 || b >= height_p)
					continue;

				float32 contrib = filter.samp_2d(a-x, b-y);
				if (contrib == 0.0)
					continue;

				accum(a,b) += contrib;
				pixels(a,b) += samp * contrib;
			}
		}
	}

	/**
	 * @brief Returns a byte array suitable for OIIO to save an
	 * 8-bit-per-channel RGB image file.
	 *
	 * The array is in scanline order.
	 *
	 * TODO: currently assumes the film will be templated from Color struct.
	 *       Remove that assumption.
	 */
	uint8 *scanline_image_8bbc(float32 gamma=2.2) {
		uint8 *im = new uint8[width*height*3];
		float32 inv_gamma = 1.0 / gamma;

		for (uint32 y=0; y < height; y++) {
			for (uint32 x=0; x < width; x++) {
				uint32 xx = x + pad;
				uint32 yy = y + pad;

				// Convert colors to 8bit gamma corrected color space
				float32 r = 0.0;
				float32 g = 0.0;
				float32 b = 0.0;
				if (accum(xx,yy) != 0.0) {
					r = pow(pixels(xx,yy)[0] / accum(xx,yy), inv_gamma) * 255;
					g = pow(pixels(xx,yy)[1] / accum(xx,yy), inv_gamma) * 255;
					b = pow(pixels(xx,yy)[2] / accum(xx,yy), inv_gamma) * 255;
				}

				// Add dither
				r += rng.next_float_c();
				g += rng.next_float_c();
				b += rng.next_float_c();

				// Clamp
				r = std::min(255.f, std::max(0.f, r));
				g = std::min(255.f, std::max(0.f, g));
				b = std::min(255.f, std::max(0.f, b));

				// Record in the byte array
				uint32 i = ((y * width) + x) * 3;
				im[i] = (uint8)(r);
				im[i+1] = (uint8)(g);
				im[i+2] = (uint8)(b);
			}
		}

		return im;
	}
};

#endif

