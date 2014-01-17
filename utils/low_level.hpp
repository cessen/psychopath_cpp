#ifndef LOW_LEVEL_HPP
#define LOW_LEVEL_HPP

#include <mmintrin.h>

template <typename T>
inline void prefetch_L1(T* address) {
	_mm_prefetch(address, _MM_HINT_T0);
}

template <typename T>
inline void prefetch_L2(T* address) {
	_mm_prefetch(address, _MM_HINT_T1);
}

template <typename T>
inline void prefetch_L3(T* address) {
	_mm_prefetch(address, _MM_HINT_T2);
}

#endif // LOW_LEVEL_HPP