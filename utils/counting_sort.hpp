#ifndef COUNTING_SORT_HPP
#define COUNTING_SORT_HPP

#include <iostream>
#include <algorithm>
#include "numtype.h"

namespace CountingSort
{

/**
 * @brief Counting sort algorithm.
 *
 * Works on any array whose items can be sorted as non-negative
 * integers (e.g. there are a finite and countable number of possible
 * values).  However, to be practical the maximum integer can't be
 * too absurdly large.
 *
 * The benefit of counting sort is that it sorts in linear time to the
 * length of the array (makes 6*array_length accesses to the data), and
 * there is extremely efficient for very large array sizes.
 *
 * @param list Pointer to the beginning of the array.
 * @param list_length Length of the array.
 * @param max_items The largest integer that can come out of an item in the array.
 * @param indexer Pointer to a function that can turn type T into an integer.
 */
template <class T>
bool sort(T *list, size_t list_length, size_t max_items, size_t(*indexer)(const T &))
{
	size_t item_counts[max_items];
	for (size_t i = 0; i < max_items; i++) {
		item_counts[i] = 0;
	}

	// Count the items
	for (size_t i = 0; i < list_length; i++) {
		item_counts[indexer(list[i])]++;
	}

	// Set up start-index array
	size_t item_start_indices[max_items];
	size_t running_count = 0;
	for (size_t i = 0; i < max_items; i++) {
		item_start_indices[i] = running_count;
		running_count += item_counts[i];
	}

	// Set up filled-so-far-count array
	size_t item_fill_counts[max_items];
	for (size_t i = 0; i < max_items; i++) {
		item_fill_counts[i] = 0;
	}

	// Sort the list
	size_t traversal = 0;
	size_t i = 0;
	while (i < list_length) {
		const size_t index = indexer(list[i]);
		const size_t next_place = item_start_indices[index] + item_fill_counts[index];

		if (i >= item_start_indices[index] && i < next_place) {
			i++;
		} else {
			std::swap(list[i], list[next_place]);
			item_fill_counts[index]++;
		}
		traversal++;
	}

	return true;
}


}
#endif // COUNTING_SORT_HPP