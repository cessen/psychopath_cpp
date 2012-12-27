#ifndef LRU_CACHE_HPP
#define LRU_CACHE_HPP

#include <iostream>
#include <stdlib.h>
#include <map>
#include <list>
#include <utility>
#include <boost/thread.hpp>
#include "config.hpp"

typedef size_t LRUKey;

template <class T>
struct LRUPair {
public:
	LRUKey key;
	T *data_ptr;
	size_t active_readers;

	LRUPair() {
		active_readers = 0;
	}
};

#define LRU_PAIR LRUPair<T>


/*
 * A Least-Recently-Used cache.
 */

template <class T>
class LRUCache
{
	boost::mutex mut;

	size_t max_bytes;
	size_t byte_count;
	LRUKey next_key;

	// A map from indices to iterators into the list
	std::map<LRUKey, typename std::list<LRU_PAIR >::iterator> map;
	// A list that contains the index and a pointer to the data of each element
	std::list<LRU_PAIR > elements;

private:
	/*
	 * Erases the given key and associated data from the cache.
	 */
	void erase(LRUKey key) {
		byte_count -= map[key]->data_ptr->bytes();
		delete map[key]->data_ptr;
		elements.erase(map[key]);
		map.erase(key);
	}

	/*
	 * Erases the last inactive element in the cache.
	 */
	void erase_last() {
		typedef typename std::list<LRU_PAIR >::reverse_iterator LRUitr;
		LRUitr rit = elements.rbegin();

		while (map[rit->key]->active_readers != 0 && rit != elements.rend())
			rit++;

		if (rit != elements.rend())
			erase(rit->key);
	}

	/*
	 * Moves a given item to the front of the cache.
	 */
	void touch(LRUKey key) {
		elements.splice(elements.begin(), elements, map[key]);
	}

public:
	LRUCache(size_t max_bytes_=40) {
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
	void set_max_size(size_t size) {
		max_bytes = size;
	}

	/*
	 * Adds the given item to the cache and opens it.
	 * Returns the key.
	 */
	LRUKey add_open(T *data_ptr) {
		boost::mutex::scoped_lock lock(mut);

		typename std::list<LRU_PAIR >::iterator it;
		LRU_PAIR data_pair;

		LRUKey key;
		do {
			key = next_key++;
		} while ((bool)(map.count(key)) || key == 0);

		byte_count += data_ptr->bytes();

		// Remove last element(s) if necessary to make room
		// TODO: this needs to be smarter, because with multiple
		// threads it's not always possible to get below max_bytes
		// or down to zero size.
		while (byte_count >= max_bytes && elements.size() > 0)
			erase_last();

		// Add the new data
		data_pair.key = key;
		data_pair.data_ptr = data_ptr;
		data_pair.active_readers = 1;
		it = elements.begin();
		it = elements.insert(it, data_pair);

		// Log it in the map
		map[key] = it;

		return key;
	}

	/**
	 * @brief Fetches the data associated with a key.
	 *
	 * You must call close() when finished with the data,
	 * so the LRU knows it can free it.
	 *
	 * @param key The key of the data to fetch.
	 *
	 * @return Pointer to the data on success, NULL if the data isn't
	 *         in the cache.
	 *
	 * Example usage:
	 * Data *p = cache.open(12345, &p)
	 * if (p) {
	 *     // Do things with the data here
	 *     cache.close(12345);
	 * }
	 */
	T *open(LRUKey key) {
		boost::mutex::scoped_lock lock(mut);

		// Check if the key exists
		const bool exists = (bool)(map.count(key));
		if (!exists)
			return NULL;

		// Increment reader count
		map[key]->active_readers++;

		return map[key]->data_ptr;
	}

	/**
	 * @brief Closes the given data key.
	 */
	void close(LRUKey key) {
		boost::mutex::scoped_lock lock(mut);

		// Assert that the key exists
		assert((bool)(map.count(key)));

		// Assert that there are readers
		assert(map[key]->active_readers > 0);

		// Decrement reader count
		map[key]->active_readers--;

		// Move data to front of the cache
		touch(key);
	}
};

#endif
