#ifndef SLICE_HPP
#define SLICE_HPP

#include <cstdlib>
#include <type_traits>

#include "array.hpp"

/**
 * @brief A slice of an Array.
 *
 * Useful for, e.g., accessing a subset of an Array.
 * Important is that this does not own the memory it accesses.  It
 * only references it.
 */
template <class T>
class Slice
{
	typedef typename std::remove_const<T>::type T_non_const;
	
public:
	Slice(): start_ {nullptr}, size_ {0} {};

	Slice(Array<T> &array): size_ {array.size()} {
		if (size_ == 0)
			start_ = nullptr;
		else
			start_ = &(array[0]);
	}

	Slice(Array<T> &array, size_t start, size_t end): size_ {end-start} {
		if (size_ == 0)
			start_ = nullptr;
		else
			start_ = &(array[start]);
	}


	/**
	 * @brief Initialized the slice to reference the given Array.
	 */
	void init_from(Array<T> &array) {
		size_ = array.size();

		if (size_ == 0)
			start_ = nullptr;
		else
			start_ = &(array[0]);
	}

	void init_from(const Array<T_non_const> &array) {
		size_ = array.size();

		if (size_ == 0)
			start_ = nullptr;
		else
			start_ = &(array[0]);
	}

	/**
	 * @brief Returns the size of the slice.
	 */
	size_t size() const {
		return size_;
	}

	/**
	 * @brief Returns pointer to the first element in the slice.
	 */
	T *begin() const {
		if (size_ == 0)
			return nullptr;
		else
			return &(start_[0]);
	}

	/**
	 * @brief Returns pointer to the last element in the slice.
	 */
	T *end() const {
		if (size_ == 0)
			return nullptr;
		else
			return &(start_[size_-1]);
	}

	/**
	 * @brief Access to array elements.
	 */
	T& operator[](const size_t n) {
		assert(n >= 0 && n < size_);
		return start_[n];
	}
	/**
	 * @brief Access to array elements.
	 */
	const T& operator[](const size_t n) const {
		assert(n >= 0 && n < size_);
		return start_[n];
	}

private:
	T *start_;
	size_t size_;
};

#endif // SLICE_HPP