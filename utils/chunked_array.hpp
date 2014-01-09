#ifndef CHUNKED_ARRAY_HPP
#define CHUNKED_ARRAY_HPP

#include <stdlib.h>
#include <iterator>





/**
 * @brief A one-dimensional array, optimized for cheap resizing.
 *
 * The memory of this array is not allocated in one contiguous area of RAM,
 * so pointers to elements of this array should not be incremented or
 * decremented.
 */
template <class T, size_t CHUNK_SIZE=1024>
class ChunkedArray
{
public:
	/**
	 * @brief A random access iterator for ChunkedArrays.
	 */
	class iterator: public std::iterator<std::random_access_iterator_tag, T>
	{
		T **chunks;
		size_t base;

	public:
		iterator() {
			chunks = nullptr;
			base = 0;
		}

		iterator(T **chunks_, size_t base_) {
			chunks = chunks_;
			base = base_;
		}



		T& operator*() {
			return chunks[base/CHUNK_SIZE][base%CHUNK_SIZE];
		}

		const T& operator*() const {
			return chunks[base/CHUNK_SIZE][base%CHUNK_SIZE];
		}

		iterator& operator++() {
			base++;
			return *this;
		}

		iterator& operator--() {
			base--;
			return *this;
		}

		iterator& operator+(const size_t i) {
			base += i;
			return *this;
		}

		iterator& operator-(const size_t i) {
			base -= i;
			return *this;
		}

		T& operator[](const size_t &i) {
			const size_t ii = base + i;
			return chunks[ii/CHUNK_SIZE][ii%CHUNK_SIZE];
		}

		const T& operator[](const size_t &i) const {
			const size_t ii = base + i;
			return chunks[ii/CHUNK_SIZE][ii%CHUNK_SIZE];
		}
	};

	class const_iterator: public std::iterator<std::random_access_iterator_tag, T>
	{
		const T * const * const chunks;
		size_t base;

	public:
		const_iterator() {
			chunks = nullptr;
			base = 0;
		}

		const_iterator(const T * const * const chunks_, size_t base_): chunks(chunks_) {
			//chunks = chunks_;
			base = base_;
		}

		const T& operator*() const {
			return chunks[base/CHUNK_SIZE][base%CHUNK_SIZE];
		}

		const_iterator& operator++() {
			base++;
			return *this;
		}

		const_iterator& operator--() {
			base--;
			return *this;
		}

		const_iterator& operator+(const size_t i) {
			base += i;
			return *this;
		}

		const_iterator& operator-(const size_t i) {
			base -= i;
			return *this;
		}

		const T& operator[](const size_t &i) const {
			const size_t ii = base + i;
			return chunks[ii/CHUNK_SIZE][ii%CHUNK_SIZE];
		}
	};

private:
	size_t element_count;
	size_t chunk_count;
	size_t chunk_pointer_count;
	T **chunks;

	/**
	 * Resizes the "chunks" pointer array to accommodate at least
	 * chunk_count_ chunks.  For internal use only.
	 *
	 * @returns The number of chunks that can be accommodated after calling
	 *          this.
	 */
	size_t resize_chunk_pointer_array(size_t chunk_count_) {
		// Calculate the old and new chunk pointer array sizes
		size_t old_size = chunk_pointer_count;
		size_t new_size = chunk_count_ / CHUNK_SIZE;
		if ((chunk_count_ % CHUNK_SIZE) > 0)
			new_size++;
		new_size *= CHUNK_SIZE;

		// Corner cases
		if (old_size == new_size) {
			return old_size;
		} else if (new_size == 0 || old_size == 0) {
			if (old_size != 0) {
				delete [] chunks;
				chunks = nullptr;
				chunk_pointer_count = 0;
			} else if (new_size != 0) {
				chunks = new T*[new_size];
				chunk_pointer_count = new_size;
			}
		}

		// General case
		T **chunks_2 = new T*[new_size];

		size_t smaller_size;
		if (new_size < old_size)
			smaller_size = new_size;
		else
			smaller_size = old_size;

		for (size_t i = 0; i < smaller_size; i++)
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
	size_t resize_chunks(size_t element_count_) {
		size_t old_size = chunk_count;
		size_t new_size = element_count_ / CHUNK_SIZE;
		if ((element_count_ % CHUNK_SIZE) > 0)
			new_size++;

		// Don't need to resize
		if (new_size == old_size) {
			return chunk_count;
		}
		// Sizing down
		else if (new_size < old_size) {
			for (size_t i = new_size; i < old_size; i++)
				delete [] chunks[i];
			resize_chunk_pointer_array(new_size);
			chunk_count = new_size;
		}
		// Sizing up
		else {
			resize_chunk_pointer_array(new_size);
			for (size_t i = old_size; i < new_size; i++)
				chunks[i] = new T[CHUNK_SIZE];
			chunk_count = new_size;
		}

		return chunk_count;
	}


public:
	ChunkedArray():
		element_count {0},
	              chunk_count {0},
	              chunk_pointer_count {0},
	              chunks {nullptr}
	{}

	ChunkedArray(size_t size_): ChunkedArray() {
		resize(size_);
	}

	~ChunkedArray() {
		for (size_t i = 0; i < chunk_count; i++)
			delete [] chunks[i];

		delete [] chunks;
	}

	T &operator[](const size_t &i) {
		assert(i < element_count);
		return chunks[i/CHUNK_SIZE][i%CHUNK_SIZE];
	}

	const T &operator[](const size_t &i) const {
		assert(i < element_count);
		return chunks[i/CHUNK_SIZE][i%CHUNK_SIZE];
	}

	size_t size() const {
		return element_count;
	}

	size_t resize(size_t size_) {
		// Don't need to resize
		if (element_count == size_)
			return size_;

		// Resize
		resize_chunks(size_);
		element_count = size_;

		return size_;
	}

	iterator begin() {
		return iterator(chunks, 0);
	}

	const_iterator cbegin() const {
		return const_iterator(chunks, 0);
	}

	iterator get_iterator(size_t base) {
		return iterator(chunks, base);
	}
};

#endif // CHUNKED_ARRAY_HPP
