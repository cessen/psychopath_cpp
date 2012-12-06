#ifndef DISK_CACHE_HPP
#define DISK_CACHE_HPP

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include "numtype.h"

namespace DiskCache
{

/**
 * A temporary file, with an interface that resembles a subset of fstream.
 */
class TemporaryFile
{
private:
	FILE* f;
public:
	TemporaryFile() {}

	~TemporaryFile() {}

	/**
	 * Opens the file.  Must be called before using the file.
	 */
	int open() {
		f = tmpfile();
		if (f) {
			setbuf(f, NULL);
			return 1;
		} else
			return 0;
	}

	/**
	 * Closes the file.
	 */
	int close() {
		return fclose(f);
	}

	void seek(long int i) {
		fseek(f, i, SEEK_SET);
	}

	void seekp(long int i) {
		seek(i);
	}

	void seekg(long int i) {
		seek(i);
	}

	unsigned int read(char* s, unsigned int n) {
		return fread(s, 1, n, f);
	}

	unsigned int write(const char* s, unsigned int n) {
		return fwrite(s, 1, n, f);
	}

	void put(char c) {
		putc(c, f);
	}

	void flush() {
		fflush(f);
	}
};


/**
 * @brief Information about a block loaded in the cache.
 */
struct BlockInfo {
	uint32 priority; // Priority in the LRU cache
	uint32 index; // Index of the block
	uint32 c_index; // Index to the first element of the cache block
	bool modified; // Whether the block has been modified in RAM
	bool used; // Whether the block is currently used
};


/**
 * @brief A disk cache.
 *
 * Stores a fixed amount of data, and stores all but a small amount of that
 * data on disk.  The parts kept in RAM are dynamically swapped to disk
 * depending on usage.
 *
 * TODO:
 * - The implementation is currently not thread-safe.  Would be cool to figure
 * out an efficient implementation for that.
 * - The implementation could allow a more efficient API, that allows the code
 * that is using the API to more directly interact with blocks of data.  e.g
 * right now every time an element is accessed, the cache has to be checked
 * and the memory address for a piece of data resolved.  But if the client
 * code could directly say, "Hey, I'll be working on this block for a while"
 * a lot of computational overhead could be avoided.  This approach may be
 * necessary for thread safety anyway.
 *
 */
template <class T, uint32 BLOCK_SIZE>
class Cache
{
private:
	uint32 priority_tally;

	uint32 e_count; // element_count
	uint32 block_count;
	uint32 cache_size;

	std::vector<T> cache; // Loaded cached data
	std::vector<BlockInfo> cache_info; // Information about the cached blocks
	std::vector<BlockInfo*> data_table; // Reference table for all data, including uncached data

	TemporaryFile data_file;

public:
	Cache() {}

	Cache(uint32 element_count_, uint32 cache_size_) {
		init(element_count_, cache_size_);
	}

	~Cache() {
		data_file.close();
	}


	/**
	 * @brief Initializes the Disk Cache.
	 *
	 * When this is finished, all data is on disk, and the in-RAM cache
	 * is empty.
	 *
	 * @param element_count The number of data elements.
	 * @param cache_size_ The max number of data blocks to hold in RAM at once.
	 */
	void init(uint32 element_count_, uint32 cache_size_) {
		priority_tally = 1;

		block_count = (element_count_ / BLOCK_SIZE) + 1;
		e_count = block_count * BLOCK_SIZE;
		cache_size = cache_size_;

		// Initialize vectors
		cache.resize(cache_size * BLOCK_SIZE);
		cache_info.resize(cache_size);
		data_table.resize(block_count);

		// Set all cache blocks to "not used"
		for (uint32 i=0; i < cache_size; i++) {
			cache_info[i].priority = 0;
			cache_info[i].modified = false;
			cache_info[i].used = false;
		}

		// Clear out the data_table pointers
		memset(&(data_table[0]), 0, sizeof(T*)*block_count);

		// Initialize the disk cache file with the appropriate size
		data_file.open();
		data_file.seekp((sizeof(T)*e_count)-1);
		data_file.put('\0');
		data_file.flush();

	}


	/**
	 * @brief Returns the block size.
	 */
	uint32 block_size() {
		return BLOCK_SIZE;
	}

	/**
	 * @brief Returns the number of elements.
	 */
	uint32 element_count() {
		return e_count;
	}

	/**
	 * Returns and increments the priority tally.
	 */
	uint32 priot() {
		return priority_tally++;
	}


	/**
	 * Unloads a block from the cache, making sure that any modifications
	 * are written back to disk.
	 */
	void unload_cache_block(uint32 cb_index) {
		const uint32 b_index = cache_info[cb_index].index;

		// Clear the reference from the table
		if (cache_info[cb_index].used)
			data_table[b_index] = NULL;

		// Check if the cache block is modified, and if so, write it to disk
		if (cache_info[cb_index].modified && cache_info[cb_index].used) {
			// Seek to the correct place in the file
			data_file.seekp(sizeof(T)*BLOCK_SIZE*b_index);

			// Write out the data block
			const T* i = &(cache[cb_index*BLOCK_SIZE]);
			data_file.write((char*)i, sizeof(T)*BLOCK_SIZE);

			// Clear modified tag
			cache_info[cb_index].modified = false;
			cache_info[cb_index].used = false;
		}

		// Set priority to zero
		cache_info[cb_index].priority = 0;
	}


	/**
	 * Loads a block from disk into the cache.
	 */
	void load_block(uint32 b_index) {
		if (!data_table[b_index]) {
			// Find the least recently used block in the cache
			uint32 cb_index = 0;
			uint32 p = cache_info[0].priority;
			for (uint32 i=1; i < cache_size; i++) {
				if (cache_info[i].priority < p) {
					cb_index = i;
					p = cache_info[i].priority;
				}
			}

			// Unload prior block at this location
			unload_cache_block(cb_index);

			// Set table reference
			data_table[b_index] = &(cache_info[cb_index]);

			// Fill block info
			data_table[b_index]->index = b_index;
			data_table[b_index]->c_index = cb_index*BLOCK_SIZE;
			data_table[b_index]->modified = false;
			data_table[b_index]->used = true;

			// Load block
			data_file.seekg(sizeof(T)*BLOCK_SIZE*b_index);
			data_file.read((char*)(&(cache[cb_index*BLOCK_SIZE])), sizeof(T)*BLOCK_SIZE);
		}

		// Set priority to the current priority tally
		data_table[b_index]->priority = priot();
	}


	/**
	 * Retreives the value of the given element index.
	 * For read only.
	 */
	const T read(uint32 i) {
		const uint32 b_index = i / BLOCK_SIZE;
		i %= BLOCK_SIZE;

		// Increment block priority if it's already loaded,
		// load it if not.
		if (data_table[b_index])
			data_table[b_index]->priority = priot();
		else
			load_block(b_index);

		return cache[data_table[b_index]->c_index+i];
	}


	/**
	 * Retreives the element at the given index.
	 * For reading and writing.
	 */
	T &get(uint32 i) {
		const uint32 b_index = i / BLOCK_SIZE;
		i %= BLOCK_SIZE;

		// Increment block priority if it's already loaded,
		// load it if not.
		if (data_table[b_index])
			data_table[b_index]->priority = priot();
		else
			load_block(b_index);

		data_table[b_index]->modified = true;

		return cache[data_table[b_index]->c_index+i];
	}

	/**
	 * Same as get().
	 */
	T &operator[](uint32 i) {
		return get(i);
	}

};



/**
 * Dummy class to do everything in RAM, without writing to disk.
 * For speed and accuracy comparisons.
 */
template<class T, uint32 BLOCK_SIZE>
class DummyCache
{
public:
	std::vector<T> data;

	DummyCache(uint32 block_count_, uint32 cache_size_) {
		data.resize(BLOCK_SIZE * block_count_);
	}

	const T read(uint32 i) {
		return data[i];
	}

	T &get(uint32 i) {
		return data[i];
	}

	T &operator[](uint32 i) {
		return data[i];
	}
};


} // end namespace


#endif // DISK_CACHE_HPP
