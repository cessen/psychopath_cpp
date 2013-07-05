#include "numtype.h"

#include "sobol.hpp"
#include "halton.hpp"
#include "rng.hpp"
#include "image_sampler.hpp"
#include "hilbert.hpp"
#include "morton.hpp"

#include <array>
#include <limits.h>
#include <stdlib.h>
#include <iostream>
#include <math.h>
#include <algorithm>


ImageSampler::ImageSampler(uint spp,
                           uint res_x, uint res_y,
                           uint seed):
	spp {spp}, res_x {res_x}, res_y {res_y}, rng {seed}, hash {seed} {

	x = 0;
	y = 0;
	s = 0;

	samp_taken = 0;
	tot_samp = spp * res_x * res_y;

	// Determine square power of two resolution to cover entire image
	uint dim = res_x > res_y ? res_x : res_y;
	uint curve_order = 1;
	curve_res = 2;
	while (curve_res < dim) {
		curve_res <<= 1;
		curve_order++;
	}
	points_traversed = 0;

	seed_offset = rng.next_uint();
}


ImageSampler::~ImageSampler()
{
}


/**
 * The logit function, scaled to approximate the probit function.
 *
 * We're using it as a close approximation to the gaussian inverse CDF,
 * since the gaussian inverse CDF (probit) has no analytic formula.
 */
float logit(float p, float width = 1.5f)
{
	p = 0.001f + (p * 0.998f);
	return logf(p/(1.0f-p)) * width * (0.6266f/4);
}

void ImageSampler::get_sample(uint32_t x, uint32_t y, uint32_t d, uint32_t ns, float *sample, uint16_t *coords)
{
	if (coords != nullptr) {
		coords[0] = x;
		coords[1] = y;
	}

	const std::array<size_t, 10> d_order {{7, 6, 5, 4, 2, 9, 8, 3, 1, 0}}; // Reorder the first several dimensions for least image variance


#define LDS_SAMP
#ifdef LDS_SAMP
#if 0
	uint32_t h = (Morton::xy2d(x,y) * spp) + (seed_offset*4096);
	const uint32_t samp_i = d + h;
	size_t i = 0;
	for (; i < ns && i < d_order.size(); ++i)
		sample[i] = Halton::sample(d_order[i], samp_i);
	for (; i < ns; ++i)
		sample[i] = Halton::sample(i, samp_i);
#else
	// Hash the x and y indices of the pixel and use that as an offset
	// into the LDS sequence.  This gives the image a more random appearance
	// before converging, which is less distracting than the LDS patterns.
	// But since within each pixel the samples are contiguous LDS sequences
	// this still gives very good convergence properties.
	// This also means that each pixel can keep drawing samples in a
	// "bottomless" kind of way, which is nice for e.g. adaptive sampling.
	uint32_t h = x ^ ((y >> 16) | (y << 16));
	const uint32_t samp_i = d + hash.get_int(h);

	// Generate the sample
	size_t i = 0;
	for (; i < ns && i < d_order.size(); ++i)
		sample[i] = Halton::sample(d_order[i], samp_i);
	for (; i < ns; ++i)
		sample[i] = Halton::sample(i, samp_i);
#endif
#else
	// Generate the sample
	for (size_t i = 0; i < ns; ++i) {
		sample[i] = rng.next_float();
	}
#endif



#define WIDTH 1.5f
	//sample[0] = logit(sample[0], WIDTH) + 0.5f;
	//sample[1] = logit(sample[1], WIDTH) + 0.5f;
	sample[0] = (sample[0] + x) / res_x;  // Return image x/y in normalized [0,1] range
	sample[1] = (sample[1] + y) / res_y;
}


/**
 * @brief Itteratively produces samples for an image.
 *
 * It provides x, y, u, v, and t coordinates always.
 * On top of that, additional coordinates can be requested via the ns
 * parameter.
 *
 * @param[out] sample A pointer where the sample is stored.
 * @param ns The number of additional coordinates to provide.
 */
//#define PROGRESSIVE_SAMPLING
#ifndef PROGRESSIVE_SAMPLING
bool ImageSampler::get_next_sample(uint32_t ns, float *sample, uint16_t *coords)
{
	//std::cout << s << " " << x << " " << y << std::endl;
	// Check if we're done
	if (points_traversed >= (curve_res*curve_res))
		return false;

	get_sample(x, y, s, ns, sample, coords);

	// increment to next sample
	samp_taken++;
	s++;
	if (s >= spp) {
		s = 0;

		// Space-filling curve traverses pixels
		do {
			Morton::d2xy(points_traversed, &x, &y);
			points_traversed++;
			if (points_traversed >= (curve_res*curve_res))
				return false;
		} while (x >= res_x || y >= res_y);
	}

	return true;
}
#else
bool ImageSampler::get_next_sample(uint32_t ns, float *sample, uint16_t *coords)
{
	//std::cout << s << " " << x << " " << y << std::endl;
	// Check if we're done
	if (points_traversed >= (curve_res*curve_res) && s >= spp)
		return false;

	get_sample(x, y, s, ns, sample, coords);

	samp_taken++;

	// Space-filling curve traverses pixels
	do {
		Morton::d2xy(points_traversed, &x, &y);
		points_traversed++;
		if (points_traversed >= (curve_res*curve_res)) {
			x = y = points_traversed = 0;

			// increment to next sample
			s++;
			if (s >= spp)
				return false;
		}
	} while (x >= res_x || y >= res_y);


	return true;
}
#endif
