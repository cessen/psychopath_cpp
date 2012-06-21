#include "numtype.h"

#include "sobol.hpp"
#include "halton.hpp"
#include "rng.hpp"
#include "image_sampler.hpp"
#include "sample.hpp"
#include "hilbert_curve.hpp"

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

	// Determine hilbert resolution to cover entire image
	uint dim = res_x > res_y ? res_x : res_y;
	uint hilbert_order = 1;
	hilbert_res = 2;
	while (hilbert_res < dim) {
		hilbert_res <<= 1;
		hilbert_order++;
	}
	points_traversed = 0;

	rng.seed(17);
}


ImageSampler::~ImageSampler()
{
}


#define LARGE_PRIME 5023

void ImageSampler::get_sample(uint32 x, uint32 y, uint32 d, Sample *sample, uint32 ns)
{
	// Offset LDS
	const uint32 samp_i = (Hilbert::xytod(hilbert_res, x, y) * LARGE_PRIME) + d;

	// Halton bases 3-13 for main samples
	sample->x = Halton::halton(samp_i, 1);
	sample->y = Halton::halton(samp_i, 2);
	sample->u = Halton::halton(samp_i, 3);
	sample->v = Halton::halton(samp_i, 4);
	sample->t = Halton::halton(samp_i, 5);

	// Sobol for everything else (Sobol is base 2,
	// so it works well with Halton > base 2
	if (sample->ns.size() != ns)
		sample->ns.resize(ns);
	for (uint32 i = 0; i < ns; i++)
		sample->ns[i] = sobol::sample(samp_i, i);
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
bool ImageSampler::get_next_sample(Sample *sample, uint32 ns)
{
	//std::cout << s << " " << x << " " << y << std::endl;
	// Check if we're done
	if (x >= res_x || y >= res_y || points_traversed >= (hilbert_res*hilbert_res))
		return false;

	get_sample(x, y, s, sample, ns);
	sample->x = (sample->x + x) / res_x;  // Return image x/y in normalized [0,1] range
	sample->y = (sample->y + y) / res_y;

	// increment to next sample
	samp_taken++;
	s++;
	if (s >= spp) {
		s = 0;

		// Hilbert curve traverses pixels
		do {
			Hilbert::dtoxy(hilbert_res, points_traversed, &x, &y);
			points_traversed++;
		} while (x >= res_x || y >= res_y);
	}

	return true;
}
