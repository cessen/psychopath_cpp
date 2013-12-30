#ifndef LRU_CACHE_HPP
#define LRU_CACHE_HPP

#include <cstdlib>
#include <unordered_map>
#include <list>
#include <memory>
#include <mutex>

#include "spinlock.hpp"

template <class T> class LRUCache;
template <class T> struct LRUPair;

typedef size_t LRUKey;


template <class T>
struct LRUPair {
public:
	LRUKey key;
	std::shared_ptr<T> data_ptr;
};


/*
 * A thread-safe Least-Recently-Used cache.
 */

template <class T>
class LRUCache
{
	SpinLock slock;

	size_t max_bytes;
	size_t byte_count;
	LRUKey next_key;

	// A map from indices to iterators into the list
	std::unordered_map<LRUKey, typename std::list<LRUPair<T>>::iterator> map;

	// A list that contains the index and a pointer to the data of each element
	std::list<LRUPair<T>> elements;

private:
	/*
	 * Erases the given key and associated data from the cache.
	 */
	void erase(LRUKey key) {
		byte_count -= map[key]->data_ptr->bytes();
		elements.erase(map[key]);
		map.erase(key);
	}

	/*
	 * Erases the last inactive element in the cache.
	 */
	bool erase_last() {
		for (auto rit = elements.rbegin(); rit != elements.rend(); ++rit) {
			erase(rit->key);
			return true;
		}
		return false;
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
		next_key = 1; // Starts at one so that 0 can mean null

		const size_t map_size = max_bytes / (sizeof(T)*4);
		map.reserve(map_size);
	}

	~LRUCache() {
	}

	/*
	 * Sets the maximum number of bytes in the cache.
	 * Should only be called once right after construction.
	 */
	void set_max_size(size_t size) {
		max_bytes = size;
	}

	/*
	 * Adds the given item to the cache, assigning it a unique key.
	 *
	 * Returns the key.
	 */
	LRUKey put(std::shared_ptr<T> data_ptr) {
		std::unique_lock<SpinLock> lock(slock);

		LRUKey key;
		do {
			key = next_key++;
		} while (map.count(key) != 0);

		byte_count += data_ptr->bytes();

		// Remove last element(s) if necessary to make room
		while (byte_count >= max_bytes) {
			if (!erase_last())
				break;
		}

		// Add the new data
		auto it = elements.begin();
		it = elements.insert(it, LRUPair<T> {key, data_ptr});

		// Log it in the map
		map[key] = it;

		return key;
	}

	/*
	 * Adds the given item to the cache using the given key.
	 * If the key already exists, the existing item will be
	 * replaced.
	 *
	 * Returns the key.
	 */
	LRUKey put(std::shared_ptr<T> data_ptr, LRUKey key) {
		std::unique_lock<SpinLock> lock(slock);

		// Check if the key exists, and erase it if it does
		const auto exists = static_cast<bool>(map.count(key));
		if (exists)
			erase(key);

		byte_count += data_ptr->bytes();

		// Remove last element(s) if necessary to make room
		while (byte_count >= max_bytes) {
			if (!erase_last())
				break;
		}

		// Add the new data
		auto it = elements.begin();
		it = elements.insert(it, LRUPair<T> {key, data_ptr});

		// Log it in the map
		map[key] = it;

		return key;
	}

	/**
	 * @brief Fetches the data associated with a key.
	 *
	 * @param key The key of the data to fetch.
	 *
	 * @return shared_ptr to the data on success, nullptr if the data isn't
	 *         in the cache.
	 *
	 * Example usage:
	 * std::shared_ptr<Data> p = cache.get(12345);
	 * if (p) {
	 *     // Do things with the data here
	 * }
	 */
	std::shared_ptr<T> get(LRUKey key) {
		std::unique_lock<SpinLock> lock(slock);

		// Check if the key exists
		const auto exists = static_cast<bool>(map.count(key));
		if (!exists)
			return nullptr;

		touch(key);

		return map[key]->data_ptr;
	}
};

#endif
