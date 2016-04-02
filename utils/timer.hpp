#ifndef PSYCHOPATH_TIMER_HPP
#define PSYCHOPATH_TIMER_HPP

#include <chrono>

template <class CLOCK=std::chrono::high_resolution_clock>
class Timer {
	std::chrono::time_point<CLOCK> start {CLOCK::now()};

public:
	/**
	 * Reports the time elapsed so far in seconds.
	 */
	float time() {
		const auto end = CLOCK::now();
		const float dur = static_cast<float>((end-start).count());
		return (dur * CLOCK::period::num) / CLOCK::period::den;
	}

	void reset() {
		start = CLOCK::now();
	}
};

#endif // PSYCHOPATH_TIMER_HPP