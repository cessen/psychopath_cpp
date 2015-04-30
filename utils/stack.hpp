#ifndef STACK_HPP
#define STACK_HPP

#include <vector>
#include <utility>

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
	 *
	 * TODO: take into account alignment.
	 */
	template <typename T>
	std::pair<T*, T*> push_frame(size_t element_count) {
		char* begin = frames.back().second;
		auto end = begin + (sizeof(T) * element_count);
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
	 * Pops the top frame off the stack.
	 *
	 * This invalidates any pointers to that stack frame's memory, as that
	 * memory may be used again for a subsequent stack frame push.
	 */
	void pop_frame() {
		frames.pop_back();
	}
};

#endif // STACK_HPP