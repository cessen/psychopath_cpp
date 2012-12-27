#ifndef JOB_QUEUE_HPP
#define JOB_QUEUE_HPP

#include <stdlib.h>
#include <vector>
#include <boost/thread.hpp>

/**
 * @brief A job queue for the producer/consumer model of managing threads.
 *
 *
 * The queue stores a maximum number of queued jobs in a ring, and is
 * first-in-first-out.
 */
template <class T>
class JobQueue
{
	std::vector<T> job_ring;
	size_t next_job;
	size_t job_count;

	size_t open_count;

	boost::mutex mut;
	boost::condition_variable full;
	boost::condition_variable empty;

public:
	// Constructos
	JobQueue() {
		job_ring.resize(1);
		next_job = 0;
		job_count = 0;
		open_count = 1;
	}
	JobQueue(size_t queue_size) {
		job_ring.resize(queue_size);
		next_job = 0;
		job_count = 0;
		open_count = 1;
	}


	/**
	 * @brief Resizes the queue.
	 *
	 * @warning Should only be called when there are no jobs in the
	 *          queue.
	 */
	void resize(size_t queue_size) {
		mut.lock();

		assert(job_count == 0);
		const size_t old_size = job_ring.size();
		job_ring.resize(queue_size);

		mut.unlock();
	}

	/**
	 * @brief Marks the queue as closed.
	 *
	 * Once the queue is closed, producers can no longer add jobs to
	 * the queue, and consumers will be notified when the queue is
	 * empty so they can terminate.
	 */
	void close() {
		mut.lock();
		open_count--;

		// Notify all threads that the queue is closed
		full.notify_all();
		empty.notify_all();

		mut.unlock();
	}

	void open() {
		mut.lock();
		open_count++;

		// Notify all threads that the queue is open
		full.notify_all();
		empty.notify_all();

		mut.unlock();
	}

	/**
	 * @brief Adds a job to the queue.
	 *
	 * @param job The job to add.
	 *
	 * @return True on success, false if the queue is closed.
	 */
	bool push(const T &job) {
		boost::unique_lock<boost::mutex> lock(mut);

		// Wait for open space in the queue
		while (job_count >= job_ring.size()) {
			if (open_count == 0)
				return false;
			else
				full.wait(lock);
		}

		// Add job to queue
		job_ring[(next_job+job_count)%job_ring.size()] = job;
		job_count++;

		// Notify waiting consumers that there's a job
		empty.notify_all();

		lock.unlock();
		return true;
	}

	/**
	 * @brief Gets the next job, removing it from the queue.
	 *
	 * @param [out] The popped job is copied into here.  Must be a
	 *              pointer to valid memory.
	 *
	 * @return True on success, false if the queue is empty and closed.
	 */
	bool pop(T *job) {
		boost::unique_lock<boost::mutex> lock(mut);

		// Wait for a job in the queue
		while (job_count == 0) {
			if (open_count == 0)
				return false;
			else
				empty.wait(lock);
		}

		// Pop the next job
		*job = job_ring[next_job];
		job_count--;
		next_job = (next_job+1) % job_ring.size();

		// Notify waiting producers that there's room for new jobs
		full.notify_all();

		lock.unlock();
		return true;
	}
};

#endif // JOB_QUEUE_HPP