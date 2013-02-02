#ifndef JOB_QUEUE_HPP
#define JOB_QUEUE_HPP

#include <cstdlib>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "ring_buffer_concurrent.hpp"

/**
 * @brief A job queue for the producer/consumer model of managing threads.
 *
 * Consumer threads are created and managed by the queue.  To use this,
 * simply add jobs to the queue and they will be processed.  All jobs
 * must be thread-safe, as multiple jobs can be processed concurrently.
 *
 * A job can be any object that is callable without parameters.  A good
 * choice is std::function<void()>
 */
template <class T>
class JobQueue
{
	RingBufferConcurrent<T> queue;
	std::vector<std::thread> threads;

	bool done;

	// A consumer thread, which watches the queue for jobs and
	// executes them.
	void run_consumer() {
		T job;
		while (pop(&job)) {
			job();
		}
	}

public:
	/**
	 * @brief Constructor.
	 *
	 * By default uses 1 thread and creates a queue 4 times the size
	 * of the thread count.
	 *
	 * @param thread_count Number of consumer threads to spawn for processing jobs.
	 * @param queue_size Size of the job queue buffer.  Zero means determine
	 *                   automatically from number of threads.
	 */
	JobQueue(size_t thread_count=1, size_t queue_size=0) {
		done = false;

		// Set up queue
		if (queue_size == 0)
			queue_size = thread_count * 4;
		queue.resize(queue_size);

		// Create and start consumer threads
		threads.resize(thread_count);
		for (auto &thread: threads)
			thread = std::thread(&JobQueue<T>::run_consumer, this);
	}

	// Destructor. Joins and deletes threads.
	~JobQueue() {
		finish();
	}


	/**
	 * @brief Marks the queue as done, and waits for all
	 *        jobs to finish.
	 *
	 * Once the queue is done, producers can no longer add jobs to
	 * the queue, and consumers will be notified when the queue is
	 * empty so they can terminate.
	 */
	void finish() {
		if (!done) {
			// Notify all threads that the queue is done
			done = true;
			queue.disallow_blocking();

			// Wait for threads to finish
			for (auto &thread: threads)
				thread.join();
		}
	}


	/**
	 * @brief Adds a job to the queue.
	 *
	 * @param job The job to add.
	 *
	 * @return True on success, false if the queue is closed.
	 */
	bool push(const T &job) {
		// Add job to queue
		return queue.push_blocking(job);
	}


	/**
	 * @brief Gets the next job, removing it from the queue.
	 *
	 * @param [out] job The popped job is copied into here.  Must be a
	 *                  pointer to valid memory.
	 *
	 * @return True on success, false if the queue is empty and closed.
	 */
	bool pop(T *job) {
		// Pop the next job
		return queue.pop_blocking(job);
	}
};

#endif // JOB_QUEUE_HPP