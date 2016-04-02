#ifndef PSYCHOPATH_SPINLOCK_HPP
#define PSYCHOPATH_SPINLOCK_HPP

#include <atomic>


/*
 ** @brief A simple spinlock.
 *
 * Useful for low-contention thread syncronization, where the lock is
 * not held for very long compared to other work done.  For locks that
 * are held for long periods of time, a mutex is generally better.
 */
class SpinLock {
	std::atomic_flag lock_flag {ATOMIC_FLAG_INIT};
public:
	/**
	 * @brief Acquires the lock, spinning until success.
	 */
	void lock() {
		while (lock_flag.test_and_set(std::memory_order_acquire));
	}

	/**
	 * @brief Attempts to acquire the lock once, returning true on
	 * success and false on failure.
	 */
	bool try_lock() {
		return !lock_flag.test_and_set(std::memory_order_acquire);
	}

	/**
	 * @brief Releases the lock.
	 */
	void unlock() {
		lock_flag.clear(std::memory_order_release);
	}
};


/**
 * @brief A reader-writer spinlock.
 *
 * Allows multiple readers to acquire the lock, but only one writer
 * at a time.  Useful for cases where writers are rare compared to
 * readers and where the locks are generally only held for short
 * periods.
 */
class SpinLockRW {
	std::atomic_flag w_lock {ATOMIC_FLAG_INIT}; // Writer lock
	std::atomic<unsigned int> r_lock_count {0}; // Reader lock count
public:
	/**
	 * @brief Acquires the writer lock, spinning until success.
	 */
	void lock_w() {
		while (w_lock.test_and_set(std::memory_order_acquire));

		while (r_lock_count > 0);
	}

	/**
	 * @brief Attempts to acquire the writer lock once, returning true
	 * on success and false on failure.
	 */
	bool try_lock_w() {
		if (!w_lock.test_and_set(std::memory_order_acquire)) {
			if (r_lock_count == 0) {
				return true;
			} else {
				w_lock.clear(std::memory_order_release);
				return false;
			}
		}
		return false;
	}

	/**
	 * @brief Releases the writer lock.
	 */
	void unlock_w() {
		w_lock.clear(std::memory_order_release);
	}

	/**
	 * @brief Acquires a reader lock, spinning until success.
	 */
	void lock_r() {
		while (w_lock.test_and_set(std::memory_order_acquire));
		++r_lock_count;
		w_lock.clear(std::memory_order_release);
	}

	/**
	 * @brief Attempts to acquire a reader lock once, returning true
	 * on success and false on failure.
	 */
	bool try_lock_r() {
		if (w_lock.test_and_set(std::memory_order_acquire))
			return false;
		++r_lock_count;
		w_lock.clear(std::memory_order_release);
		return true;
	}

	/**
	 * @brief Releases a reader lock.
	 */
	void unlock_r() {
		--r_lock_count;
	}
};

#endif // PSYCHOPATH_SPINLOCK_HPP