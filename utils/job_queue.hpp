#ifndef JOB_QUEUE_HPP
#define JOB_QUEUE_HPP

#include <cstdlib>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "ring_buffer.hpp"

/**
 * @brief A job queue for the producer/consumer model of managing threads.
 *
 * Consumer threads are created and managed by the queue.  To use this,
 * simply add jobs to the queue and they will be processed.  All jobs
 * must be thread-safe, as multiple jobs can be processed concurrently.
 *
 * A job can be any object that is callable without parameters.
 */
template <class T>
class JobQueue
{
	RingBuffer<T> queue;
	std::vector<std::thread> threads;

	bool done;

	std::mutex mut;
	std::condition_variable full;
	std::condition_variable empty;

	// A consumer thread, which watches the queue for jobs and
	// executes them.
	void consumer_run() {
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
		for (size_t i = 0; i < threads.size(); i++)
			threads[i] = std::thread(&JobQueue<T>::consumer_run, this);
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
		mut.lock();
		if (!done) {
			// Notify all threads that the queue is done
			done = true;
			full.notify_all();
			empty.notify_all();

			mut.unlock();

			// Wait for threads to finish, then delete them
			for (size_t i = 0; i < threads.size(); i++) {
				threads[i].join();
			}
		} else {
			mut.unlock();
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
		std::unique_lock<std::mutex> lock(mut);

		// Wait for open space in the queue
		while (queue.is_full()) {
			if (done)
				return false;
			else
				full.wait(lock);
		}

		// Add job to queue
		queue.push(job);

		// Notify waiting consumers that there's a job
		empty.notify_all();

		return true;
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
		std::unique_lock<std::mutex> lock(mut);

		// Wait for a job in the queue
		while (queue.is_empty()) {
			if (done)
				return false;
			else
				empty.wait(lock);
		}

		// Pop the next job
		*job = queue.pop();

		// Notify waiting producers that there's room for new jobs
		full.notify_all();

		return true;
	}
};

#endif // JOB_QUEUE_HPP