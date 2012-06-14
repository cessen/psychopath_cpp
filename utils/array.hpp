#ifndef ARRAY_HPP
#define ARRAY_HPP

#include <assert.h>
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
	typedef uint32 size_type;
	size_type size_;
	size_type capacity_;
	T *data;

public:
	Array() {
		size_ = 0;
		capacity_ = 0;
		data = 0;
	}

	Array(size_type size) {
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
	void reserve(size_type cap) {
		assert(cap > 0);

		if (cap <= capacity_)
			return;

		T *data2 = new T[cap];

		for (size_type i=0; i < size_; i++)
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
	void resize(size_type size) {
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
	const size_type &capacity() const {
		return capacity_;
	}

	/**
	 * @brief Returns the current size of the array.
	 */
	const size_type &size() const {
		return size_;
	}

	/**
	 * @brief Access to array elements.
	 */
	T& operator[](const size_type &n) {
		assert(n >= 0 && n < size_);
		return data[n];
	}
	/**
	 * @brief Access to array elements.
	 */
	const T& operator[](const size_type &n) const {
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
