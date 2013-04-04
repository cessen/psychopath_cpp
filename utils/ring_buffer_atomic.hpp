#ifndef RING_BUFFER
#define RING_BUFFER

#include <cstdlib>
#include <vector>
#include <atomic>


template <class T>
class RingBufferAtomicItem
{
	T item;
	std::atomic_flag taken;
};


/**
 * WIP
 * @brief A ring buffer, or circular buffer, that uses atomics to be
 * thread-safe for consumers.  Only single-producer is supported at
 * the moment.
 *
 * Acts as a limited-size FIFO queue, where overflow simply results in
 * the queue overwriting itself from the back.
 */
template <class T>
class RingBufferAtomic
{
private:
	std::vector<RingBufferAtomicItem<T>> buffer;

	std::atomic<size_t> next;  // Index of the next item to be consumed
	std::atomic<size_t> count;  // Number of unconsumed items in the buffer

public:
	/**
	 * @brief Default constructor, buffer size of 1.
	 */
	RingBuffer(): buffer(1), next {0}, count {0} {
		for (auto& item: buffer)
			item.taken.test_and_set(std::memory_order_acquire);
	}

	/**
	 * @brief Constructor.
	 *
	 * @param size Size of the buffer in number-of-items.
	 */
	RingBuffer(size_t buffer_size): buffer(buffer_size), next {0}, count {0} {
		for (auto& item: buffer)
			item.taken.test_and_set(std::memory_order_acquire);
	}

	/**
	 * @brief Resizes the buffer.
	 *
	 * @warning Significant data loss and/or loss of proper
	 * syncronization between threads can happen if this is called
	 * at the wrong time.  Only call this before any reading or writing
	 * is done.
	 *
	 * @param size New size of the buffer in number-of-items
	 */
	void resize(size_t buffer_size) {
		next = 0;
		count = 0;
		buffer.resize(buffer_size);
		for (auto& item: buffer)
			item.taken.test_and_set(std::memory_order_acquire);
	}

	/**
	 * @brief Returns the size of the buffer.
	 */
	size_t size() const {
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
		buffer[(next+count)%buffer.size()].item = item;
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