#include "numtype.h"

#include "sobol.hpp"
#include "rng.hpp"
#include "image_sampler.hpp"
#include "sample.hpp"
#include "hilbert_curve.h"

#include <limits.h>
#include <stdlib.h>
#include <iostream>
#include <math.h>
#include <algorithm>

inline float32 radical_inverse(int n, int base)
{
	float32 val = 0;
	float32 inv_base = 1.0 / base;
	float32 inv_bi = inv_base;
	while (n > 0) {
		int d_i = (n % base);
		val += d_i * inv_bi;
		n /= base;
		inv_bi *= inv_base;
	}
	return val;
}


ImageSampler::ImageSampler(uint spp_,
                           uint res_x_, uint res_y_,
                           float32 f_width_,
                           uint bucket_size_)
{
	spp = spp_;
	res_x = res_x_;
	res_y = res_y_;
	f_width = ceilf(f_width_) * 2;
	bucket_size = bucket_size_;

	x = 0;
	y = 0;
	s = 0;

	samp_taken = 0;
	tot_samp = spp * res_x * res_y;

	// Determine hilbert order to cover entire image
	uint dim = res_x > res_y ? res_x : res_y;
	dim += f_width;
	hilbert_order = 1;
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


bool ImageSampler::get_next_sample(Sample *sample)
{
	//std::cout << s << " " << x << " " << y << std::endl;
	// Check if we're done
	if (x >= res_x+f_width || y >= res_y+f_width || points_traversed >= (hilbert_res*hilbert_res))
		return false;

	// Using sobol
	///*
	sample->x = (sobol::sample(samp_taken, 0) + x) / res_x;
	sample->y = (sobol::sample(samp_taken, 1) + y) / res_y;
	sample->u = sobol::sample(samp_taken, 2);
	sample->v = sobol::sample(samp_taken, 3);
	sample->t = sobol::sample(samp_taken, 4);
	//*/

	// Using random
	/*
	sample->x = (rng.next_float() + x) / res_x;
	sample->y = (rng.next_float() + y) / res_y;
	sample->u = rng.next_float();
	sample->v = rng.next_float();
	sample->t = rng.next_float();
	*/

	// increment to next sample
	samp_taken++;
	s++;
	if (s >= spp) {
		s = 0;

		// Hilbert curve traverses pixels
		do {
			hil_inc_xy(&x, &y, hilbert_order);
			points_traversed++;
		} while (x >= res_x || y >= res_y);
	}

	return true;
}
