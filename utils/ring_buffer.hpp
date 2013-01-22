#ifndef RING_BUFFER
#define RING_BUFFER

#include <cstdlib>
#include <vector>

/**
 * @brief A ring buffer, or circular buffer.
 *
 * Acts as a limited-size FIFO queue, where overflow simply results in
 * the queue overwriting itself from the back.
 */
template <class T>
class RingBuffer
{
private:
	std::vector<T> buffer;

	size_t next;  // Index of the next item to be consumed
	size_t count;  // Number of unconsumed items in the buffer

public:
	/**
	 * @brief Default constructor, buffer size of 1.
	 */
	RingBuffer() {
		next = 0;
		count = 0;
		buffer.resize(1);
	}

	/**
	 * @brief Constructor.
	 *
	 * @param size Size of the buffer in number-of-items.
	 */
	RingBuffer(size_t buffer_size) {
		next = 0;
		count = 0;
		buffer.resize(buffer_size);
	}

	/**
	 * @brief Resizes the buffer.
	 *
	 * @warning Significant data loss can occur if this is done on
	 *          a non-empty buffer.  Check is_empty() before calling
	 *          this.
	 *
	 * @param size New size of the buffer in number-of-items
	 *
	 * TODO: minimize data loss when running this.
	 */
	void resize(size_t buffer_size) {
		next = 0;
		count = 0;
		buffer.resize(buffer_size);
	}

	/**
	 * @brief Returns the size of the buffer.
	 */
	size_t size() {
		return buffer.size();
	}

	/**
	 * @brief Pushes an item onto the front of the buffer.
	 *
	 * If the buffer is full, this will start over-writing
	 * the tail of the buffer.  Make sure to check is_full()
	 * if you don't want this behavior.
	 *
	 * @param item The item to push.
	 */
	void push(const T &item) {
		buffer[(next+count)%buffer.size()] = item;
		count++;

		// If we overwrote a non-empty item in the buffer
		if (count > buffer.size()) {
			next = (next + 1) % buffer.size();
			count = buffer.size();
		}
	}

	/**
	 * @brief Pops an item off the back of the buffer.
	 *
	 * If the buffer is empty, this will return garbage.
	 * Make sure to check is_empty().
	 *
	 * @return The popped item.
	 */
	T pop() {
		const size_t i = next;
		if (count > 0) {
			next = (next + 1) % buffer.size();
			count--;
		}

		return buffer[i];
	}

	/**
	 * @brief Returns whether the buffer is full or not.
	 */
	bool is_full() {
		return count == buffer.size();
	}

	/**
	 * @brief Returns whether the buffer is empty or not.
	 */
	bool is_empty() {
		return count == 0;
	}
};

#endif // RING_BUFFER