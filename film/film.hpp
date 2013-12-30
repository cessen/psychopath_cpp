#ifndef FILM_HPP
#define FILM_HPP

#include <stdlib.h>

#include "numtype.h"

#include <algorithm>
#include <assert.h>

#include "morton.hpp"
#include "color.hpp"
#include "rng.hpp"
#include "blocked_array_disk_cache.hpp"

#define LBS 5


/**
 * @brief Maps a linear brightness range to a range that approximates
 * the human eye's sensitivity to brightness.
 */
static float hcol(float n)
{
	if (n < 0.0f)
		n = 0.0f;
	return log((n*100)+1) / log(101);
}
static Color hcol(Color n)
{
	return Color(hcol(n[0]), hcol(n[1]), hcol(n[2]));
}


/**
 * @brief Calculates the absolute difference between two values.
 */
static float diff(float n1, float n2)
{
	return fabs(n1-n2);
}
static Color diff(Color c1, Color c2)
{
	Color c = Color(diff(c1[0], c2[0]),
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
static Color mmax(Color a, Color b)
{
	return Color(mmax(a[0], b[0]), mmax(a[1], b[1]), mmax(a[2], b[2]));
}


/**
 * Film that accumulates samples while rendering.
 *
 * Along with the mean of the samples, a "variance" value is also maintained.
 * It's not proper variance in the Normal Distribution sense, but it seems to
 * be better at representing the potential for noise in the image.
 * See add_sample() for details.
 */
template <class PIXFMT>
class Film
{
public:
	RNG rng;
	uint16_t width, height; // Resolution of the image in pixels
	float min_x, min_y; // Minimum x/y coordinates of the image
	float max_x, max_y; // Maximum x/y coordinates of the image

	BlockedArrayDiskCache<PIXFMT, LBS> pixels; // Pixel data
	BlockedArrayDiskCache<uint16_t, LBS> accum; // Accumulation buffer
	BlockedArrayDiskCache<PIXFMT, LBS> var_p; // Entropy buffer "previous"
	BlockedArrayDiskCache<PIXFMT, LBS> var_f; // Entropy buffer "final"

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
				pixels(u,v) = PIXFMT(0);
				accum(u,v) = 0;
				var_p(u,v) = PIXFMT(0);
				var_f(u,v) = PIXFMT(0);
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
	void add_sample(PIXFMT samp, uint x, uint y) {
		pixels(x,y) += samp;
		accum(x,y)++;

		// Update "variance"
		const uint16_t k = accum(x,y);
		const PIXFMT avg = hcol(pixels(x,y) / k);
		if (k > 1)
			var_f(x,y) += diff(var_p(x,y), avg) * (k-1);
		var_p(x,y) = avg;
	}

	/**
	 * Returns a maximal estimate of the variance of the pixel.
	 *
	 * The intent is for this to give a good idea of the amount
	 * of "noise" that a pixel contributes to the image, which
	 * is useful for e.g. adaptive sampling.
	 *
	 * The estimate is calculated by taking the maximum
	 * esimated variance from a surrounding block of pixels.
	 * The size of the block depends on the sample count of
	 * the current pixel: lower sample counts lead to larger
	 * block sizes, to minimize the chances of underestimating
	 * variance with low sample counts.
	 * Finally, that maximum is divided by the square root of
	 * the number of samples in the pixel, so that the estimate
	 * decreases appropriately as the samples increase.
	 */
	PIXFMT variance_estimate(int32_t x, int32_t y) {
		// Calculate block size
		uint r;
		if (accum(x,y) < 2)
			r = 0;
		else
			r = (uint)(11.0f / sqrt(accum(x,y)) + 0.9f);

		// Calculate block extents
		uint i1, j1, i2, j2;
		i1 = std::max((int32_t)(0), x-(int32_t)(r));
		i2 = std::min((int32_t)(width), x+(int32_t)(r));
		j1 = std::max((int32_t)(0), y-(int32_t)(r));
		j2 = std::min((int32_t)(height), y+(int32_t)(r));

		// Get the largest variance estimate within the block
		PIXFMT result = PIXFMT(0);
		for (uint i = i1; i <= i2; i++) {
			for (uint j = j1; j <= j2; j++) {
				if (accum(i,j) > 1) {
					PIXFMT t = var_f(i,j) / (accum(i,j)-1);
					result = mmax(result, t);
				}
			}
		}

		// Variance is reduced by increased number of samples
		return result / sqrt(accum(x,y));
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
	std::vector<uint8_t> scanline_image_8bbc(float gamma=2.2) {
		auto im = std::vector<uint8_t>(width*height*3);
		float inv_gamma = 1.0 / gamma;

		for (uint32_t y=0; y < height; y++) {
			for (uint32_t x=0; x < width; x++) {
				float r = 0.0;
				float g = 0.0;
				float b = 0.0;

//#define VARIANCE
#ifdef VARIANCE
				const PIXFMT var_est = variance_estimate(x, y);
				r = var_est[0];
				g = var_est[1];
				b = var_est[2];
#else
				// Get color and apply gamma correction
				if (accum(x,y) != 0.0) {
					r = pixels(x,y)[0] / accum(x,y);
					g = pixels(x,y)[1] / accum(x,y);
					b = pixels(x,y)[2] / accum(x,y);
					r = pow(r, inv_gamma);
					g = pow(g, inv_gamma);
					b = pow(b, inv_gamma);
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

