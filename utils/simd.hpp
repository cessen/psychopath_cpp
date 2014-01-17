#ifndef SIMD_HPP
#define SIMD_HPP

#include <x86intrin.h>

namespace SIMD
{

struct float4 {
	__m128 data;

	float4(): data(_mm_setzero_ps()) {};
	float4(const float f): data(_mm_set_ps1(f)) {}
	float4(const float f1, const float f2, const float f3, const float f4): data(_mm_set_ps(f4, f3, f2, f1)) {}
	float4(const float* const fs): data(_mm_load_ps(fs)) {}
	float4(const __m128& s): data(s) {}

	float& operator[](const int i) {
		float* fs = reinterpret_cast<float*>(&data);
		return fs[i];
	}
	const float& operator[](const int i) const {
		const float* fs = reinterpret_cast<const float*>(&data);
		return fs[i];
	}
};


inline float4 operator+(const float4& a, const float4& b)
{
	return float4(_mm_add_ps(a.data, b.data));
}

inline float4 operator-(const float4& a, const float4& b)
{
	return float4(_mm_sub_ps(a.data, b.data));
}

inline float4 operator*(const float4& a, const float4& b)
{
	return float4(_mm_mul_ps(a.data, b.data));
}

inline float4 operator*(const float4& a, const float b)
{
	return float4(_mm_mul_ps(a.data, _mm_set_ps1(b)));
}

inline float4 operator/(const float4& a, const float4& b)
{
	return float4(_mm_div_ps(a.data, b.data));
}

inline float4 operator/(const float4& a, const float b)
{
	return float4(_mm_div_ps(a.data, _mm_set_ps1(b)));
}

inline float4 eq(const float4& a, const float4& b)
{
	return float4(_mm_cmpeq_ps(a.data, b.data));
}

inline float4 lt(const float4& a, const float4& b)
{
	return float4(_mm_cmplt_ps(a.data, b.data));
}

inline float4 gt(const float4& a, const float4& b)
{
	return float4(_mm_cmpgt_ps(a.data, b.data));
}

inline float4 lte(const float4& a, const float4& b)
{
	return float4(_mm_cmple_ps(a.data, b.data));
}

inline float4 gte(const float4& a, const float4& b)
{
	return float4(_mm_cmpge_ps(a.data, b.data));
}

inline float4 operator&&(const float4& a, const float4& b)
{
	return float4(_mm_and_ps(a.data, b.data));
}


inline float4 min(const float4& a, const float4& b)
{
	return float4(_mm_min_ps(a.data, b.data));
}

inline float4 max(const float4& a, const float4& b)
{
	return float4(_mm_max_ps(a.data, b.data));
}

/**
 * @brief Swaps the left and right pair of floats in a SSE float4 vector.
 *
 * Can be turned into a no-op by setting the "swap" parameter to false.
 */
inline float4 shuffle_swap(const float4& a, bool swap=true)
{
	static const unsigned int shuf_swap = (1<<6) | (0<<4) | (3<<2) | 2; // Shuffle parameter for swapping
	if (swap)
		return float4(_mm_shuffle_ps(a.data, a.data, shuf_swap));
	else
		return a;
}

inline unsigned int to_bitmask(const float4& a)
{
	return _mm_movemask_ps(a.data);
}

}

#endif // SIMD_HPP