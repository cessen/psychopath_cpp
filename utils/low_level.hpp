#ifndef LOW_LEVEL_HPP
#define LOW_LEVEL_HPP

#include <mmintrin.h>

namespace LowLevel
{

static const int cache_line_size = 64;

template <typename T>
inline void prefetch_L1(T* address)
{
	constexpr int lines = (sizeof(T)/cache_line_size) + ((sizeof(T)%cache_line_size) == 0 ? 0 : 1);
	for (int i = 0; i < lines; ++i) {
		_mm_prefetch(address+i, _MM_HINT_T0);
	}
}

template <typename T>
inline void prefetch_L2(T* address)
{
	constexpr int lines = (sizeof(T)/cache_line_size) + ((sizeof(T)%cache_line_size) == 0 ? 0 : 1);
	for (int i = 0; i < lines; ++i) {
		_mm_prefetch(address+i, _MM_HINT_T1);
	}
}

template <typename T>
inline void prefetch_L3(T* address)
{
	constexpr int lines = (sizeof(T)/cache_line_size) + ((sizeof(T)%cache_line_size) == 0 ? 0 : 1);
	for (int i = 0; i < lines; ++i) {
		_mm_prefetch(address+i, _MM_HINT_T2);
	}
}

}

#endif // LOW_LEVEL_HPP