#ifndef BLOCKED_ARRAY_DISK_CACHE_HPP
#define BLOCKED_ARRAY_DISK_CACHE_HPP

#include <iostream>
#include <vector>

#include "morton.hpp"
#include "disk_cache.hpp"

#define BLOCK_CACHE_SIZE 64

/**
 * @brief A 2d array optimized for cache coherency, and which pages
 * large data to disk.
 *
 * TODO: This class is currently NOT thread safe, even for reading.  This
 * should be addressed in the DiskCache class eventually.
 *
 */
template <class T, uint32 LOG_BLOCK_SIZE>
class BlockedArrayDiskCache
{
private:
	uint32 block_size, block_mask;
	uint32 u_blocks, v_blocks;
	uint32 block_elements;

	DiskCache<T, (1<<LOG_BLOCK_SIZE)> data;

public:
	uint32 width, height;

	BlockedArrayDiskCache() {}

	BlockedArrayDiskCache(uint32 w, uint32 h) {
		init(w, h);
	}

	~BlockedArrayDiskCache() {}

	void init(uint32 w, uint32 h) {
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

		data.init(width*height, BLOCK_CACHE_SIZE);
	}

	uint32 index(uint32 u, uint32 v) const {
		// Find the start of the block
		const uint32 bu = u >> LOG_BLOCK_SIZE;
		const uint32 bv = v >> LOG_BLOCK_SIZE;
		const uint32 i1 = block_elements * ((bv * u_blocks) + bu);

		// Find the index within the block
		u &= block_mask;
		v &= block_mask;
		const uint32 i2 = Morton::xy2d(u, v);

		return i1 + i2;
	}

	// Element addressing
	T &operator()(uint32 u, uint32 v) {
		return data[index(u, v)];
	}

	const T &operator()(uint32 u, uint32 v) const {
		return data[index(u, v)];
	}

};

#endif // BLOCKED_ARRAY_DISK_CACHE_HPP
