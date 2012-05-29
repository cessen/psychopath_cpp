#ifndef LRU_CACHE_H
#define LRU_CACHE_H

#include "numtype.h"

#include <iostream>
#include <stdlib.h>
#include <map>
#include <list>
#include <utility>
#include "config.hpp"

typedef uint32 LRUKey;

template <class T>
class LRUPair
{
public:
	LRUKey key;
	T *data_ptr;
};

#define LRU_PAIR LRUPair<T>


/*
 * A Least-Recently-Used cache.
 */

template <class T>
class LRUCache
{
private:
	uint32 max_bytes;
	uint32 byte_count;
	LRUKey next_key;

	// A map from indices to iterators into the list
	std::map<LRUKey, typename std::list<LRU_PAIR >::iterator> map;
	// A list that contains the index and a pointer to the data of each element
	std::list<LRU_PAIR > elements;

public:
	LRUCache(uint32 max_bytes_=40) {
		max_bytes = max_bytes_;
		byte_count = 0;
		next_key = 1; // Starts at one so that 0 can mean NULL
	}

	~LRUCache() {
		typename std::list<LRU_PAIR >::iterator it;

		for (it = elements.begin(); it != elements.end(); it++) {
			delete it->data_ptr;
		}
	}


	/*
	 * Sets the maximum number of bytes in the cache.
	 * Should only be called once right after construction.
	 */
	void set_max_size(uint32 size) {
		max_bytes = size;
	}


	/*
	 * Erases the given key and associated data from the cache.
	 */
	void erase(LRUKey key) {
		//std::cout << "Erasing grid from LRU... "; std::cout.flush();

		byte_count -= map[key]->data_ptr->bytes();
		delete map[key]->data_ptr;
		elements.erase(map[key]);
		map.erase(key);

		//std::cout << "done." << std::endl; std::cout.flush();
	}


	/*
	 * Erases the last element in the cache.
	 */
	void erase_last() {
		LRUKey key = elements.rbegin()->key;
		erase(key);
	}


	/*
	 * Adds the given item to the cache.
	 * Returns the key.
	 */
	LRUKey add(T *data_ptr) {
		//std::cout << "Adding grid to LRU." << std::endl; std::cout.flush();
		typename std::list<LRU_PAIR >::iterator it;
		LRU_PAIR data_pair;

		LRUKey key;
		do {
			key = next_key++;
		} while (exists(key, false) || key == 0);

		byte_count += data_ptr->bytes();

		// Remove last element(s) if necessary to make room
		while (byte_count >= max_bytes && elements.size() > 0)
			erase_last();

		// Add the new data
		data_pair.key = key;
		data_pair.data_ptr = data_ptr;
		it = elements.begin();
		it = elements.insert(it, data_pair);

		// Log it in the map
		map[key] = it;

		return key;
	}


	/*
	 * Returns whether the given key exists in the Cache or not.
	 */
	bool exists(LRUKey key, bool inc_miss=true) {
		bool x = (bool)(map.count(key));

		if (inc_miss && key != 0 && !x)
			Config::cache_misses++;

		return x;
	}


	/*
	 * Return a pointer to the data associated with the given key.
	 */
	T* fetch(LRUKey key) {
		//std::cout << map[key]->data_ptr << std::endl;
		if (exists(key))
			return map[key]->data_ptr;
		else
			return NULL;
	}


	/*
	 * Returns a pointer to the data associated with the given
	 * key.
	 */
	T *operator[](uint32 key) {
		return fetch(key);
	}


	/*
	 * Moves a given item to the front of the cache.
	 */
	void touch(LRUKey key) {
		elements.splice(elements.begin(), elements, map[key]);
	}
};

#endif
