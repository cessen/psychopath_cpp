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

///**
// * @brief Pixel filter.
// *
// * Implements a mitchel filter and a box filter, switched with a define.
// */
//class Filter
//{
//#define MITCHEL
//#ifdef MITCHEL
//	float32 B, C;

//public:
//	Filter(float32 C_ = 0.5) {
//		C = C_;
//		B = 1.0 - (2*C);
//	}

//	float32 width() {
//		return 2.0;
//	}

//	float32 samp_1d(float32 x) {
//		x = std::abs(x);
//		if (x > 2.f)
//			return 0.f;
//		if (x > 1.f)
//			return ((-B - 6*C) * x*x*x + (6*B + 30*C) * x*x +
//			        (-12*B - 48*C) * x + (8*B + 24*C)) * (1.f/6.f);
//		else
//			return ((12 - 9*B - 6*C) * x*x*x +
//			        (-18 + 12*B + 6*C) * x*x +
//			        (6 - 2*B)) * (1.f/6.f);
//	}

//	float32 samp_2d(float32 x, float32 y) {
//		return samp_1d(x) * samp_1d(y);
//	}
//#else
//	float32 W;
//public:
//	Filter(float32 W_ = 0.5) {
//		W = W_;
//	}

//	float32 width() {
//		return W;
//	}

//	float32 samp_1d(float32 x) {
//		if(x > W || x < -W)
//			return 0.0f;
//		else
//			return 1.0f;
//	}

//	float32 samp_2d(float32 x, float32 y) {
//		return samp_1d(x) * samp_1d(y);
//	}
//#endif
//};


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

	BlockedArrayDiskCache<PIXFMT, LBS> pixels; // Pixel data
	BlockedArrayDiskCache<uint16, LBS> accum; // Accumulation buffer

	/**
	 * @brief Constructor.
	 *
	 * Creates a new Film.  All pixel values are initialized to a zeroed state.
	 */
	Film(int w, int h, int padding, float32 x1, float32 y1, float32 x2, float32 y2) {
		// Store meta data
		width = w;
		height = h;
		min_x = std::min(x1, x2);
		min_y = std::min(y1, y2);
		max_x = std::max(x1, x2);
		max_y = std::max(y1, y2);

		// Allocate pixel and accum data
		pixels.init(width, height);
		accum.init(width, height);

		// Zero out pixels and accum
		std::cout << "Clearing out\n";
		uint32 u = 0;
		uint32 v = 0;
		for (uint32 i = 0; u < width || v < height; i++) {
			Morton::d2xy(i, &u, &v);
			if (u < width && v < height) {
				pixels(u,v) = PIXFMT(0);
				accum(u,v) = 0;
			}
		}

		// Set up RNG
		rng = RNG(7373546);
	}


	/**
	 * @brief Adds a sample to the film.
	 */
	void add_sample(PIXFMT samp, uint x, uint y) {
		accum(x,y)++;
		pixels(x,y) += samp;
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
				// Convert colors to 8bit gamma corrected color space
				float32 r = 0.0;
				float32 g = 0.0;
				float32 b = 0.0;
				if (accum(x,y) != 0.0) {
					r = pow(pixels(x,y)[0] / accum(x,y), inv_gamma) * 255;
					g = pow(pixels(x,y)[1] / accum(x,y), inv_gamma) * 255;
					b = pow(pixels(x,y)[2] / accum(x,y), inv_gamma) * 255;
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

