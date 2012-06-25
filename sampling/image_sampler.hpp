#ifndef IMAGE_SAMPLER_HPP
#define IMAGE_SAMPLER_HPP

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
	int bucket_size;  // Width and height of each bucket in pixels

	/* State information. */
	uint curve_res; // Space filling curve resolution
	uint points_traversed;
	uint32 x, y, s;

	/* For reporting percentages. */
	uint samp_taken;
	uint tot_samp;

	/* Random number generator. */
	RNG rng;


public:
	ImageSampler(uint spp_,
	             uint res_x_, uint res_y_,
	             uint bucket_size_=0);
	~ImageSampler();

	void init_tile();
	void get_sample(uint32 x, uint32 y, uint32 d, Sample *sample, uint32 ns);
	bool get_next_sample(Sample *sample, uint32 ns);

	float percentage() const {
		return ((float)(samp_taken)) / tot_samp;
	}
};

#endif
