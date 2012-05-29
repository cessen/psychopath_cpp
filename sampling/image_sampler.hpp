#ifndef IMAGE_SAMPLER_H
#define IMAGE_SAMPLER_H

#include "numtype.h"

#include "sample.hpp"
#include "rng.hpp"
#include <vector>

/*
 * An image sampler.  Returns samples for use by the renderer.
 * Image plane <x,y> samples are returned on the [0,1] square, + edge buffer for filtering.
 * Lens <u,v> samples are returned on the [0,1) square.
 * Time samples are returned on the [0,1) line.
 * All 1d, 2d, and 3d samples are returned on the [0,1) line, square,
 * and cube respectively.
 * The renderer is expected to transform sample ranges as necessary.
 */
class ImageSampler
{
private:
	/* General settings. */
	uint spp;  // Approximate number of samples per pixel
	uint res_x, res_y;  // Image resolution in pixels
	float32 f_width;  // Filter width; not used directly
	// here, but we need to know so we can buffer
	// the image around the edges
	int bucket_size;  // Width and height of each bucket in pixels

	/* State information. */
	uint hilbert_order, hilbert_res;
	uint points_traversed;
	uint x, y, s;

	/* For reporting percentages. */
	uint samp_taken;
	uint tot_samp;

	/* Random number generator. */
	RNG rng;


public:
	ImageSampler(uint spp_,
	             uint res_x_, uint res_y_,
	             float32 f_width_=1.0,
	             uint bucket_size_=0);
	~ImageSampler();

	void init_tile();
	bool get_next_sample(Sample *sample);

	float percentage() const {
		return ((float)(samp_taken)) / tot_samp;
	}
};

#endif
