#ifndef STACK_HPP
#define STACK_HPP

#include <vector>
#include <utility>
#include <cassert>
#include <stdint.h>

/**
 * A type-erased stack that can store arrays of POD data.
 *
 * Do _not_ use this to store RAII types, as their destructors
 * will not be run.  Also, you must keep track of the types
 * you store yourself.
 */
class Stack
{
	std::vector<char> data;
	std::vector<std::pair<char*, char*>> frames;

public:
	Stack() = delete;
	Stack(size_t data_capacity, size_t reserved_frames): data(data_capacity) {
		frames.reserve(reserved_frames+1);
		frames.emplace_back(std::make_pair(&(data[0]), &(data[0])));
	}


	/**
	 * Pushes space for element_count items or type T, and returns pointers to
	 * the beginning and just-past-the-end of the resulting array.
	 */
	template <typename T>
	std::pair<T*, T*> push_frame(size_t element_count) {
		// Figure out how much padding we need between elements for proper
		// memory alignment if we put them in an array.
		constexpr auto array_pad = (alignof(T) - (sizeof(T) % alignof(T))) % alignof(T);

		// Total needed bytes for the requested array of data
		const auto needed_bytes = (sizeof(T) * element_count) + (array_pad * (element_count - 1));

		// Figure out how much padding we need at the beginning to put the
		// first element in the right place for memory alignment.
		const auto mem_addr = reinterpret_cast<uintptr_t>(frames.back().second);
		const auto begin_pad = (alignof(T) - (mem_addr % alignof(T))) % alignof(T);

		// Push onto the stack
		char* begin = reinterpret_cast<char*>(mem_addr) + begin_pad;
		auto end = begin + needed_bytes;
		frames.emplace_back(std::make_pair(begin, end));

		return std::make_pair(reinterpret_cast<T*>(begin), reinterpret_cast<T*>(end));
	}

	/**
	 * Returns the top frame, as pointers with the specified type T.
	 */
	template <typename T>
	std::pair<T*, T*> top_frame() {
		return std::make_pair(reinterpret_cast<T*>(frames.back().first), reinterpret_cast<T*>(frames.back().second));
	}

	/**
	 * Returns a frame walking backwards from the top frame.  Zero means the
	 * top frame.
	 */
	template <typename T>
	std::pair<T*, T*> prev_frame(size_t i) {
		assert(i < frames.size());
		const auto i2 = frames.size() - i - 1;
		return std::make_pair(reinterpret_cast<T*>(frames[i2].first), reinterpret_cast<T*>(frames[i2].second));
	}

	/**
	 * Pops the top frame off the stack.
	 *
	 * This invalidates any pointers to that stack frame's memory, as that
	 * memory may be used again for a subsequent stack frame push.
	 */
	void pop_frame() {
		frames.pop_back();
	}

	/**
	 * Clears the stack, as if no pushes had ever taken place.
	 */
	void clear() {
		frames.clear();
		frames.emplace_back(std::make_pair(&(data[0]), &(data[0])));
	}
};

#endif // STACK_HPP