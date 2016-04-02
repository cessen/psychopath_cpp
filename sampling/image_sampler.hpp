#ifndef IMAGE_SAMPLER_HPP
#define IMAGE_SAMPLER_HPP

#include "numtype.h"

#include "halton.hpp"
#include "rng.hpp"
#include "hash.hpp"
#include <vector>



/**
 * A sampler for a single "item" which requires a multi-dimensional sample.
 */
struct Sampler {
	uint32_t offset;
	uint32_t dim = 0;

	Sampler(): offset {0} {}
	Sampler(uint32_t x, uint32_t y, uint32_t n, uint32_t seed) {
		offset = hash_u32(x ^ ((y >> 16) | (y << 16)), seed) + n;
	}

	float get_sample(const uint32_t dimension) const {
		static const std::array<size_t, 11> d_order {{10, 7, 6, 5, 4, 2, 9, 8, 3, 1, 0}}; // Reorder the first several dimensions for least image variance

		if (dimension < d_order.size()) {
			return Halton::sample(d_order[dimension], offset);
		} else {
			return Halton::sample(dimension, offset);
		}
	}

	float next() {
		return get_sample(dim++);
	}
};


/*
 * An image sampler.  Returns samples for use by the renderer.
 * Image plane <x,y> samples are returned on the [0,1] square, + edge buffer for filtering.
 * Lens <u,v> samples are returned on the [0,1) square.
 * Time samples are returned on the [0,1) line.
 * All 1d, 2d, and 3d samples are returned on the [0,1) line, square,
 * and cube respectively.
 * The renderer is expected to transform sample ranges as necessary.
 */
class ImageSampler {
private:
	/* General settings. */
	uint spp;  // Approximate number of samples per pixel
	uint res_x, res_y;  // Image resolution in pixels

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
	uint32_t seed;

public:
	ImageSampler(uint spp,
	             uint res_x, uint res_y,
	             uint seed=0);
	~ImageSampler();

	void init_tile();
	Sampler get_single_sampler(uint32_t x, uint32_t y, uint32_t i) {
		return Sampler(x, y, i, seed);
	}
	void get_sample(uint32_t x, uint32_t y, uint32_t d, uint32_t ns, float *sample, uint16_t *coords=nullptr);
	bool get_next_sample(uint32_t ns, float *sample, uint16_t *coords=nullptr);

	float percentage() const {
		return ((float)(samp_taken)) / tot_samp;
	}
};



#endif
