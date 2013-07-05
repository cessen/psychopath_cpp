#ifndef IMAGE_SAMPLER_HPP
#define IMAGE_SAMPLER_HPP

#include "numtype.h"

#include "rng.hpp"
#include "hash.hpp"
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
	uint32_t seed_offset;

	/* State information. */
	uint curve_res; // Space filling curve resolution
	uint points_traversed;
	uint32_t x, y, s;

	/* For reporting percentages. */
	uint samp_taken;
	uint tot_samp;

	/* Random number generator. */
	RNG rng;
	Hash hash;

public:
	ImageSampler(uint spp,
	             uint res_x, uint res_y,
	             uint seed=0);
	~ImageSampler();

	void init_tile();
	void get_sample(uint32_t x, uint32_t y, uint32_t d, uint32_t ns, float *sample, uint16_t *coords=nullptr);
	bool get_next_sample(uint32_t ns, float *sample, uint16_t *coords=nullptr);

	float percentage() const {
		return ((float)(samp_taken)) / tot_samp;
	}
};

#endif
