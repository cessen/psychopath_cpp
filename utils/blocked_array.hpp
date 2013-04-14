#ifndef BLOCKED_ARRAY_HPP
#define BLOCKED_ARRAY_HPP

#include <iostream>
#include <vector>

#include "morton.hpp"

/**
 * @brief A 2d array optimized for cache coherency.
 */
template <class T, uint32_t LOG_BLOCK_SIZE>
class BlockedArray
{
private:
	uint32_t block_size, block_mask;
	uint32_t u_blocks, v_blocks;
	uint32_t block_elements;

	std::vector<T> data;

public:
	uint32_t width, height;

	BlockedArray() {}

	BlockedArray(uint32_t w, uint32_t h) {
		block_size = 1 << LOG_BLOCK_SIZE;
		block_mask = block_size - 1;
		width = w;
		height = h;

		// Round width and height up to the nearest multiple of block_size
		if (width % block_size)
			width = width - (width % block_size) + block_size;
		if (height % block_size)
			height = height - (height % block_size) + block_size;

		// Calculate the number of blocks in the horizontal direction
		u_blocks = width >> LOG_BLOCK_SIZE;

		// Calculate the number of elements in a block
		block_elements = block_size * block_size;

		data.resize(width*height);
	}

	~BlockedArray() {}

	uint32_t index(uint32_t u, uint32_t v) const {
		// Find the start of the block
		const uint32_t bu = u >> LOG_BLOCK_SIZE;
		const uint32_t bv = v >> LOG_BLOCK_SIZE;
		const uint32_t i1 = block_elements * ((bv * u_blocks) + bu);

		// Find the index within the block
		u &= block_mask;
		v &= block_mask;
		const uint32_t i2 = Morton::xy2d(u, v);

		return i1 + i2;
	}

	// Element addressing
	T &operator()(uint32_t u, uint32_t v) {
		return data[index(u, v)];
	}

	const T &operator()(uint32_t u, uint32_t v) const {
		return data[index(u, v)];
	}

};

#endif // BLOCKED_ARRAY_HPP
