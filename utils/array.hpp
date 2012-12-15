#ifndef ARRAY_HPP
#define ARRAY_HPP

#include <assert.h>
#include <stdlib.h>
#include "numtype.h"

/**
 * @brief Essentially a custom implementation of standard vector.
 *
 * This way I can rely on very specific behaviors, regardless of compiler
 * or platform.
 */
template <class T>
class Array
{
	uint_i size_;
	uint_i capacity_;
	T *data;

public:
	Array() {
		size_ = 0;
		capacity_ = 0;
		data = 0;
	}

	Array(uint_i size) {
		data = new T[size];
		size_ = size;
		capacity_ = size;
	}

	~Array() {
		if (data)
			delete [] data;
	}


	/**
	 * @brief Increase capacity of the array.
	 *
	 * This does _not_ shrink the capacity, only increases.
	 * If cap is less then the current capacity, it does nothing.
	 */
	void reserve(uint_i cap) {
		assert(cap > 0);

		if (cap <= capacity_)
			return;

		T *data2 = new T[cap];

		for (uint_i i=0; i < size_; i++)
			data2[i] = data[i];

		delete [] data;
		data = data2;
		capacity_ = cap;
	}

	/**
	 * @brief Resize the array.
	 *
	 * This does _not_ free any space.  The entire capacity is left alone.
	 */
	void resize(uint_i size) {
		if (size > capacity_)
			reserve(size);

		size_ = size;
	}

	/**
	 * @brief Sets the array size to zero.
	 */
	void clear() {
		size_ = 0;
	}

	/**
	 * @brief Returns the current capacity of the array.
	 */
	const uint_i &capacity() const {
		return capacity_;
	}

	/**
	 * @brief Returns the current size of the array.
	 */
	const uint_i &size() const {
		return size_;
	}

	/**
	 * @brief Returns pointer to the first element in the array.
	 */
	T *begin() const {
		if (size_ == 0)
			return NULL;
		else
			return &(data[0]);
	}

	/**
	 * @brief Returns pointer to the last element in the array.
	 */
	T *end() const {
		if (size_ == 0)
			return NULL;
		else
			return &(data[size_-1]);
	}

	/**
	 * @brief Access to array elements.
	 */
	T& operator[](const uint_i &n) {
		assert(n >= 0 && n < size_);
		return data[n];
	}
	/**
	 * @brief Access to array elements.
	 */
	const T& operator[](const uint_i &n) const {
		assert(n >= 0 && n < size_);
		return data[n];
	}

	/**
	 * @brief Adds a new element to the end of the array.
	 */
	void push_back(const T& el) {
		resize(size_+1);
		data[size_-1] = el;
	}
};

#endif // ARRAY_HPP
