#ifndef RANGE_HPP
#define RANGE_HPP

#include <iterator>

/**
 * A Range class, based on the proposal for std::range at:
 * http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2012/n3350.html
 * But with a few things omitted.
 */
template<typename Iterator>
class Range
{
private:
	Iterator iter_begin = nullptr;
	Iterator iter_end = nullptr;

public:
	// types
	typedef typename std::iterator_traits<Iterator>::iterator_category iterator_category;
	typedef typename std::iterator_traits<Iterator>::value_type value_type;
	typedef typename std::iterator_traits<Iterator>::difference_type difference_type;
	typedef typename std::iterator_traits<Iterator>::reference reference;
	typedef typename std::iterator_traits<Iterator>::pointer pointer;

	// constructors
	Range() {}
	constexpr Range(Iterator begin, Iterator end): iter_begin {begin}, iter_end {end} {}

	// iterator access
	constexpr Iterator begin() const {
		return iter_begin;
	}
	constexpr Iterator end() const {
		return iter_end;
	}

	// element access
	constexpr reference front() const {
		return *iter_begin;
	}
	constexpr reference back() const {
		return *iter_end;
	}
	constexpr reference operator[](difference_type index) const {
		return *(iter_begin + index);
	}

	// size
	constexpr bool empty() const {
		return iter_begin == iter_end;
	}
	constexpr difference_type size() const {
		return std::distance(iter_begin, iter_end);
	}

	// creating derived ranges
	//pair< range, range > split(difference_type index) const;
	//Range slice(difference_type start, difference_type stop) const;
	//Range slice(difference_type start) const;
};

// deducing constructor wrappers
template<typename Iterator>
constexpr Range<Iterator> make_range(Iterator begin, Iterator end)
{
	return Range<Iterator>(begin, end);
}

#endif // RANGE_HPP
