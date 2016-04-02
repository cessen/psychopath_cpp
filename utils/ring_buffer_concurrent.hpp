#ifndef RING_BUFFER_CONCURRENT
#define RING_BUFFER_CONCURRENT

#include <cstdlib>
#include <atomic>
#include <mutex>
#include <condition_variable>

#include "ring_buffer.hpp"

/**
 * @brief A thread-safe ring buffer, or circular buffer.
 *
 * Acts as a limited-size FIFO queue.
 */
template <class T>
class RingBufferConcurrent {
private:
	RingBuffer<T> buffer;

	std::mutex mut;
	std::condition_variable full;
	std::condition_variable empty;

	bool stop;
	std::atomic<size_t> blocker_count;  // Counters for blocking pushers and poppers.

public:
	/**
	 * @brief Default constructor, buffer size of 1.
	 */
	RingBufferConcurrent(): buffer(1), stop {false}, blocker_count {0} {}

	/**
	 * @brief Constructor.
	 *
	 * @param size Size of the buffer in number-of-items.
	 */
	RingBufferConcurrent(size_t buffer_size): buffer(buffer_size), stop {false}, blocker_count {0} {}

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
		std::unique_lock<std::mutex> lock(mut);
		buffer.resize(buffer_size);
	}

	/**
	 * @brief Returns the size of the buffer.
	 */
	size_t size() {
		return buffer.size();
	}

	/**
	 * @brief Forces current blocking calls to end and return false.
	 *
	 * Any currently waiting call to push_blocking() or pop_blocking()
	 * will be stopped and will return false.
	 */
	void stop_blocking() {
		mut.lock();
		stop = true;
		full.notify_all();
		empty.notify_all();
		mut.unlock();

		// Wait for all blockers to stop
		while (blocker_count > 0) {}

		mut.lock();
		stop = false;
		mut.unlock();
	}

	/**
	 * @brief Stops all blocking calls and prevents further blocking
	 * calls.
	 */
	void disallow_blocking() {
		mut.lock();
		stop = true;
		full.notify_all();
		empty.notify_all();
		mut.unlock();
	}

	/**
	 * @brief Pushes an item onto the front of the buffer.
	 *
	 * @param [in] item The item to push.
	 *
	 * @return Whether the item was successfully pushed or not.
	 */
	bool push(const T &item) {
		std::unique_lock<std::mutex> lock(mut);
		if (buffer.is_full())
			return false;

		// Push item
		buffer.push(item);

		// Notify waiting poppers that there's an item in the queue
		empty.notify_all();

		return true;
	}

	/**
	 * @brief Pushes an item onto the front of the buffer.
	 *
	 * If the buffer is full, this will block until there is space
	 * to successfully push.
	 *
	 * @param [in] item The item to push.
	 *
	 * @return Whether the item was successfully pushed or not.
	 */
	bool push_blocking(const T &item) {
		std::unique_lock<std::mutex> lock(mut);
		blocker_count++;

		// Wait for open space in the buffer
		while (buffer.is_full()) {
			if (stop) {
				blocker_count--;
				return false;
			} else {
				full.wait(lock);
			}
		}

		// Push item
		buffer.push(item);

		// Notify waiting poppers that there's an item in the queue
		empty.notify_all();

		blocker_count--;
		return true;
	}

	/**
	 * @brief Pops an item off the back of the buffer.
	 *
	 * @param [out] item Popped item is copied to this memory location.
	 * @return Whether an item was successfully popped or not.
	 */
	bool pop(T* item) {
		std::unique_lock<std::mutex> lock(mut);
		if (buffer.is_empty())
			return false;

		// Pop item
		*item = buffer.pop();

		// Notify waiting pushers that there's space free
		full.notify_all();

		return true;
	}

	/**
	 * @brief Pops an item off the back of the buffer.
	 *
	 * If the buffer is empty, this will block until there is an item
	 * to pop.
	 *
	 * @param [out] item Popped item is copied to this memory location.
	 * @return Whether an item was popped or not.
	 */
	bool pop_blocking(T* item) {
		std::unique_lock<std::mutex> lock(mut);
		blocker_count++;

		// Wait for open space in the buffer
		while (buffer.is_empty()) {
			if (stop) {
				blocker_count--;
				return false;
			} else {
				empty.wait(lock);
			}
		}

		// Pop item
		*item = buffer.pop();

		// Notify waiting pushers that there's space free
		full.notify_all();

		blocker_count--;
		return true;
	}
};

#endif // RING_BUFFER_CONCURRENT