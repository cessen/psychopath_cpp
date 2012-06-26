#include "numtype.h"

#include "sobol.hpp"
#include "halton.hpp"
#include "rng.hpp"
#include "image_sampler.hpp"
#include "sample.hpp"
#include "hilbert.hpp"
#include "morton.hpp"

#include <limits.h>
#include <stdlib.h>
#include <iostream>
#include <math.h>
#include <algorithm>


ImageSampler::ImageSampler(uint spp_,
                           uint res_x_, uint res_y_,
                           uint bucket_size_)
{
	spp = spp_;
	res_x = res_x_;
	res_y = res_y_;
	bucket_size = bucket_size_;

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

	rng.seed(8758796);
}


ImageSampler::~ImageSampler()
{
}


void ImageSampler::get_sample(uint32 x, uint32 y, uint32 d, Sample *sample, uint32 ns)
{
	// These gymnastics are so that different pixels map to very different
	// and random-ish LDS indices.  It gives the image a more random appearance
	// before converging, which is less distracting than the LDS patterns.
	// But since within each pixel the samples are contiguous LDS sequences
	// this still gives very good convergence properties.
	// This also means that each pixel can keep drawing samples in a
	// "bottomless" kind of way, which is nice for e.g. adaptive sampling.
	const uint32 temp = ((y*73571) + x) * 3885701021;
	const uint32 index = ((temp*1936502639)^(temp*1264700623));
	const uint32 samp_i = index + d;

	// Generate the sample
	sample->x = Halton::sample(5, samp_i);
	sample->y = Halton::sample(4, samp_i);
	sample->u = Halton::sample(3, samp_i);
	sample->v = Halton::sample(2, samp_i);
	sample->t = Halton::sample(1, samp_i);
	if (sample->ns.size() != ns)
		sample->ns.resize(ns);
	for (uint32 i = 0; i < ns; i++) {
		sample->ns[i] = Sobol::sample(i, samp_i);
	}

	sample->x = (sample->x + x) / res_x;  // Return image x/y in normalized [0,1] range
	sample->y = (sample->y + y) / res_y;
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
#define PROGRESSIVE_SAMPLING
#ifndef PROGRESSIVE_SAMPLING
bool ImageSampler::get_next_sample(Sample *sample, uint32 ns)
{
	//std::cout << s << " " << x << " " << y << std::endl;
	// Check if we're done
	if (points_traversed >= (curve_res*curve_res))
		return false;

	get_sample(x, y, s, sample, ns);

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
bool ImageSampler::get_next_sample(Sample *sample, uint32 ns)
{
	//std::cout << s << " " << x << " " << y << std::endl;
	// Check if we're done
	if (points_traversed >= (curve_res*curve_res) && s >= spp)
		return false;

	get_sample(x, y, s, sample, ns);

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
