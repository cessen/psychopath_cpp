#ifndef FILM_HPP
#define FILM_HPP

#include <stdlib.h>

#include "numtype.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <tuple>
#include <assert.h>

#include "morton.hpp"
#include "color.hpp"
#include "rng.hpp"
#include "blocked_array.hpp"

#define LBS 5


/**
 * @brief Maps a linear brightness range to a range that approximates
 * the human eye's sensitivity to brightness.
 */
static float hcol(float n)
{
	if (n < 0.0f)
		n = 0.0f;
	return pow(n, 1.0f/2.2f);
}
static Color_XYZ hcol(Color_XYZ n)
{
	return Color_XYZ(hcol(n[0]), hcol(n[1]), hcol(n[2]));
}


/**
 * @brief Calculates the absolute difference between two values.
 */
static float diff(float n1, float n2)
{
	return fabs(n1-n2);
}
static Color_XYZ diff(Color_XYZ c1, Color_XYZ c2)
{
	Color_XYZ c = Color_XYZ(diff(c1[0], c2[0]),
	                        diff(c1[1], c2[1]),
	                        diff(c1[2], c2[2]));
	return c;
}


/**
 * @brief Returns the maximum of two variables.
 *
 * In the case of multi-component variables, it returns a variable
 * with each component being individually maximized.
 */
static float mmax(float a, float b)
{
	if (a > b)
		return a;
	else
		return b;
}
static Color_XYZ mmax(Color_XYZ a, Color_XYZ b)
{
	return Color_XYZ(mmax(a[0], b[0]), mmax(a[1], b[1]), mmax(a[2], b[2]));
}


/**
 * Film that accumulates samples while rendering.
 *
 * Along with the mean of the samples, a "variance" value is also maintained.
 * It's not proper variance in the Normal Distribution sense, but it seems to
 * be better at representing the potential for noise in the image.
 * See add_sample() for details.
 *
 * TODO: currently Film only collects color data.  Should be expanded to handle
 * render layers and AOV's.
 */
class Film
{
public:
	RNG rng;
	uint16_t width, height; // Resolution of the image in pixels
	float min_x, min_y; // Minimum x/y coordinates of the image
	float max_x, max_y; // Maximum x/y coordinates of the image

	BlockedArray<Color_XYZ, LBS> pixels; // Pixel data
	BlockedArray<uint16_t, LBS> accum; // Accumulation buffer
	BlockedArray<Color_XYZ, LBS> var_p; // Entropy buffer "previous"
	BlockedArray<Color_XYZ, LBS> var_f; // Entropy buffer "final"

	/**
	 * @brief Constructor.
	 *
	 * Creates a new Film.  All pixel values are initialized to a zeroed state.
	 */
	Film(uint16_t w, uint16_t h, float x1, float y1, float x2, float y2):
		rng {RNG(7373546)},
	    width {w}, height {h},
	    min_x {std::min(x1, x2)}, min_y {std::min(y1, y2)},
	max_x {std::max(x1, x2)}, max_y {std::max(y1, y2)} {
		// Allocate pixel and accum data
		pixels.init(width, height);
		accum.init(width, height);
		var_p.init(width, height);
		var_f.init(width, height);

		// Zero out pixels and accum
		//std::cout << "Clearing out\n";
		uint32_t u = 0;
		uint32_t v = 0;
		for (uint32_t i = 0; u < width || v < height; i++) {
			Morton::d2xy(i, &u, &v);
			if (u < width && v < height) {
				pixels(u,v) = Color_XYZ(0);
				accum(u,v) = 0;
				var_p(u,v) = Color_XYZ(0);
				var_f(u,v) = Color_XYZ(0);
			}
		}
	}

	/**
	 * @brief Adds a sample to the film.
	 *
	 * The "variance" is calculated by keeping a running sum of how much each
	 * sample changes the mean, compensating for the inherent lowering of that
	 * effect as more samples are accumulated.  That sum is then divided by the
	 * sample count minus one to get the "variance".
	 * It's ad-hoc as far as I know, but the idea is that if on average the
	 * samples are not changing the mean very much, then there isn't much
	 * opportunity for noise to be introduced.
	 */
	void add_sample(Color_XYZ samp, uint x, uint y) {
		// Skip NaN and infinite samples
		for (int i = 0; i < 3; ++i) {
			if (std::isnan(samp[i]) || !std::isfinite(samp[i])) {
				// TODO: log when this happens
				return;
			}
		}

		pixels(x,y) += samp;
		accum(x,y)++;

		// Update "variance"
		const uint16_t k = accum(x,y);
		const Color_XYZ avg = hcol(pixels(x,y) / k);
		if (k > 1)
			var_f(x,y) += diff(var_p(x,y), avg) * (k-1);
		var_p(x,y) = avg;
	}

	/**
	 * Returns an estimate of the variance of the pixel.
	 */
	Color_XYZ variance_estimate(int32_t x, int32_t y) {
		const int samps = accum(x,y);

		if (samps < 2)
			return std::numeric_limits<float>::infinity();
		else
			return (var_f(x,y) / (samps-1)) / sqrt(samps);
	}

	/**
	 * @brief Returns a byte array suitable for OIIO to save an
	 * 8-bit-per-channel RGB image file.
	 *
	 * Output color space is sRGB.  The array is in scanline order.
	 */
	std::vector<uint8_t> scanline_image_8bbc() {
		auto im = std::vector<uint8_t>(width*height*3);

		for (uint32_t y=0; y < height; y++) {
			for (uint32_t x=0; x < width; x++) {
				float r = 0.0f;
				float g = 0.0f;
				float b = 0.0f;

				// Image shows a grey checkerboard pattern where no samples
				// have been taken.
				if (((y % 32) < 16) ^ ((x % 32) < 16))
					r = g = b = 0.5f;
				else
					r = g = b = 0.35f;

//#define VARIANCE
#ifdef VARIANCE
				const Color_XYZ var_est = variance_estimate(x, y);
				std::tie(r, g, b) = XYZ_to_sRGB_E(var_est);
#else
				// Get color and apply gamma correction
				if (accum(x,y) != 0.0) {
					Color_XYZ cxyz(pixels(x,y)[0] / accum(x,y), pixels(x,y)[1] / accum(x,y), pixels(x,y)[2] / accum(x,y));
					std::tie(r, g, b) = XYZ_to_sRGB_E(cxyz);
				}
#endif

				// Map [0,1] to [0,255]
				r *= 255;
				g *= 255;
				b *= 255;

				// Add dither
				r += rng.next_float_c();
				g += rng.next_float_c();
				b += rng.next_float_c();

				// Clamp
				r = std::min(255.f, std::max(0.f, r));
				g = std::min(255.f, std::max(0.f, g));
				b = std::min(255.f, std::max(0.f, b));

				// Record in the byte array
				uint32_t i = ((y * width) + x) * 3;
				im[i] = (uint8_t)(r);
				im[i+1] = (uint8_t)(g);
				im[i+2] = (uint8_t)(b);
			}
		}

		return im;
	}
};

#endif

