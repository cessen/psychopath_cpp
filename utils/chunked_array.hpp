#ifndef CHUNKED_ARRAY_HPP
#define CHUNKED_ARRAY_HPP

#include <stdlib.h>
#include <iterator>
#include "numtype.h"


/**
 * @brief A random access iterator for ChunkedArrays.
 */
template <class T, uint_i CHUNK_SIZE>
class ChunkedArrayIterator: public std::iterator<std::random_access_iterator_tag, T>
{
	T **chunks;
	uint_i base;

public:
	ChunkedArrayIterator() {
		chunks = NULL;
		base = 0;
	}

	ChunkedArrayIterator(T **chunks_, uint_i base_) {
		chunks = chunks_;
		base = base_;
	}



	T operator*() {
		return chunks[base/CHUNK_SIZE][base%CHUNK_SIZE];
	}

	ChunkedArrayIterator &operator++() {
		base++;
		return *this;
	}

	ChunkedArrayIterator &operator--() {
		base--;
		return *this;
	}

	T &operator[](const uint_i &i) {
		const uint_i ii = base + i;
		return chunks[ii/CHUNK_SIZE][ii%CHUNK_SIZE];
	}

	const T &operator[](const uint_i &i) const {
		const uint_i ii = base + i;
		return chunks[ii/CHUNK_SIZE][ii%CHUNK_SIZE];
	}
};


/**
 * @brief A one-dimensional array, optimized for cheap resizing.
 *
 * The memory of this array is not allocated in one contiguous area of RAM,
 * so pointers to elements of this array should not be incremented or
 * decremented.
 */
template <class T, uint_i CHUNK_SIZE>
class ChunkedArray
{
private:
	uint_i element_count;
	uint_i chunk_count;
	uint_i chunk_pointer_count;
	T **chunks;

	/**
	 * Resizes the "chunks" pointer array to accommodate at least
	 * chunk_count_ chunks.  For internal use only.
	 *
	 * @returns The number of chunks that can be accommodated after calling
	 *          this.
	 */
	uint_i resize_chunk_pointer_array(uint_i chunk_count_) {
		// Calculate the old and new chunk pointer array sizes
		uint_i old_size = chunk_pointer_count;
		uint_i new_size = chunk_count_ / CHUNK_SIZE;
		if ((chunk_count_ % CHUNK_SIZE) > 0)
			new_size++;
		new_size *= CHUNK_SIZE;

		// Corner cases
		if (old_size == new_size) {
			return old_size;
		} else if (new_size == 0 || old_size == 0) {
			if (old_size != 0) {
				delete [] chunks;
				chunks = NULL;
				chunk_pointer_count = 0;
			} else if (new_size != 0) {
				chunks = new T*[new_size];
				chunk_pointer_count = new_size;
			}
		}

		// General case
		T **chunks_2 = new T*[new_size];

		uint_i smaller_size;
		if (new_size < old_size)
			smaller_size = new_size;
		else
			smaller_size = old_size;

		for (uint_i i = 0; i < smaller_size; i++)
			chunks_2[i] = chunks[i];

		delete [] chunks;
		chunks = chunks_2;
		chunk_pointer_count = new_size;

		return chunk_pointer_count;
	}


	/**
	 * Resizes the number of chunks to accommodate at least
	 * element_count_ elements.
	 */
	uint_i resize_chunks(uint_i element_count_) {
		uint_i old_size = chunk_count;
		uint_i new_size = element_count_ / CHUNK_SIZE;
		if ((element_count_ % CHUNK_SIZE) > 0)
			new_size++;

		// Don't need to resize
		if (new_size == old_size) {
			return chunk_count;
		}
		// Sizing down
		else if (new_size < old_size) {
			for (uint_i i = new_size; i < old_size; i++)
				delete [] chunks[i];
			resize_chunk_pointer_array(new_size);
			chunk_count = new_size;
		}
		// Sizing up
		else {
			resize_chunk_pointer_array(new_size);
			for (uint_i i = old_size; i < new_size; i++)
				chunks[i] = new T[CHUNK_SIZE];
			chunk_count = new_size;
		}

		return chunk_count;
	}


public:
	ChunkedArray() {
		element_count = 0;
		chunk_count = 0;
		chunk_pointer_count = 0;
		chunks = NULL;
	}

	ChunkedArray(uint_i size_) {
		element_count = 0;
		chunk_count = 0;
		chunk_pointer_count = 0;
		chunks = NULL;

		resize(size_);
	}

	~ChunkedArray() {
		for (uint_i i = 0; i < chunk_count; i++)
			delete [] chunks[i];

		delete [] chunks;
	}

	T &operator[](const uint_i &i) {
		assert(i < element_count);
		return chunks[i/CHUNK_SIZE][i%CHUNK_SIZE];
	}

	const T &operator[](const uint_i &i) const {
		assert(i < element_count);
		return chunks[i/CHUNK_SIZE][i%CHUNK_SIZE];
	}

	uint_i size() const {
		return element_count;
	}

	uint_i resize(uint_i size_) {
		// Don't need to resize
		if (element_count == size_)
			return size_;

		// Resize
		resize_chunks(size_);
		element_count = size_;

		return size_;
	}

	ChunkedArrayIterator<T, CHUNK_SIZE> get_iterator() {
		return ChunkedArrayIterator<T, CHUNK_SIZE>(chunks, 0);
	}

	ChunkedArrayIterator<T, CHUNK_SIZE> get_iterator(uint_i base) {
		return ChunkedArrayIterator<T, CHUNK_SIZE>(chunks, base);
	}
};

#endif // CHUNKED_ARRAY_HPP
