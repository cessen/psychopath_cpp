#ifndef LRU_CACHE_HPP
#define LRU_CACHE_HPP

#include <cstdlib>
#include <unordered_map>
#include <list>
#include <memory>
#include <mutex>

#include "spinlock.hpp"


typedef size_t LRUKey;


// Should be overloaded for more complex types
template <class T>
size_t size_in_bytes(const T& data)
{
	return sizeof(T);
}

/*
 * A thread-safe Least-Recently-Used cache.
 */
template <class T>
class LRUCache
{
	struct LRUPair {
		LRUKey key;
		std::shared_ptr<T> data_ptr;
	};

	SpinLock slock;

	size_t max_bytes;
	size_t byte_count {0};
	LRUKey next_key {0};

	// A map from indices to iterators into the list
	std::unordered_map<LRUKey, typename std::list<LRUPair>::iterator> map;

	// A list that contains the index and a pointer to the data of each element
	std::list<LRUPair> elements;

	// The number of bytes each item takes up, aside from the size of the item itself.
	// In other words, the overhead of the LRUCache per-item.
	// Estimated for now as the size of an LRUPair plus the size of
	// 2 pointers (for the list) plus the size of an LRUKEY and list
	// iterator (for the map).
	// TODO: more accurate estimate
	const size_t per_item_size_cost = sizeof(LRUPair) + (sizeof(void*)*2) + sizeof(LRUKey) + sizeof(typename std::list<LRUPair>::iterator);

public:
	LRUCache(size_t max_bytes_=40): max_bytes {max_bytes_} {}

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

		// Get the next available key
		LRUKey key;
		do {
			key = next_key++;
		} while (map.count(key) != 0);

		// Add data to the cache using that key
		add(data_ptr, key);

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

		// Add data to the cache
		add(data_ptr, key);

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

private:
	/*
	 * Adds an item to the cache with the given key.
	 */
	void add(std::shared_ptr<T>& data_ptr, LRUKey key) {
		byte_count += size_in_bytes(*data_ptr) + per_item_size_cost;

		// Remove last element(s) if necessary to make room
		while (byte_count >= max_bytes) {
			if (!erase_last())
				break;
		}

		// Add the new data
		auto it = elements.begin();
		it = elements.insert(it, LRUPair {key, data_ptr});

		// Log it in the map
		map[key] = it;
	}

	/*
	 * Erases the given key and associated data from the cache.
	 */
	void erase(LRUKey key) {
		byte_count -= size_in_bytes(*(map[key]->data_ptr)) + per_item_size_cost;
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
};

#endif
