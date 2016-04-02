#ifndef SIMD_HPP
#define SIMD_HPP

#include <x86intrin.h>

namespace SIMD {

struct float4 {
	__m128 data;

	float4(): data() {};
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

	float sum() const {
		//return data[0] + data[1] + data[2] + data[3];
		__m128 s = _mm_hadd_ps(data, data);
		s = _mm_hadd_ps(s, s);
		return s[0];
	}
};


inline float4 operator+(const float4& a, const float4& b) {
	return float4(_mm_add_ps(a.data, b.data));
}

inline float4 operator-(const float4& a, const float4& b) {
	return float4(_mm_sub_ps(a.data, b.data));
}

inline float4 operator*(const float4& a, const float4& b) {
	return float4(_mm_mul_ps(a.data, b.data));
}

inline float4 operator*(const float4& a, const float b) {
	return float4(_mm_mul_ps(a.data, _mm_set_ps1(b)));
}

inline float4 operator/(const float4& a, const float4& b) {
	return float4(_mm_div_ps(a.data, b.data));
}

inline float4 operator/(const float4& a, const float b) {
	return float4(_mm_div_ps(a.data, _mm_set_ps1(b)));
}

inline float4 eq(const float4& a, const float4& b) {
	return float4(_mm_cmpeq_ps(a.data, b.data));
}

inline float4 lt(const float4& a, const float4& b) {
	return float4(_mm_cmplt_ps(a.data, b.data));
}

inline float4 gt(const float4& a, const float4& b) {
	return float4(_mm_cmpgt_ps(a.data, b.data));
}

inline float4 lte(const float4& a, const float4& b) {
	return float4(_mm_cmple_ps(a.data, b.data));
}

inline float4 gte(const float4& a, const float4& b) {
	return float4(_mm_cmpge_ps(a.data, b.data));
}

inline float4 operator&&(const float4& a, const float4& b) {
	return float4(_mm_and_ps(a.data, b.data));
}


inline float4 min(const float4& a, const float4& b) {
	return float4(_mm_min_ps(a.data, b.data));
}

inline float4 max(const float4& a, const float4& b) {
	return float4(_mm_max_ps(a.data, b.data));
}

/**
 * @brief Swaps the left and right pair of floats in a SSE float4 vector.
 *
 * Can be turned into a no-op by setting the "swap" parameter to false.
 */
inline float4 shuffle_swap(const float4& a, bool swap=true) {
	static const unsigned int shuf_swap = (1<<6) | (0<<4) | (3<<2) | 2; // Shuffle parameter for swapping
	if (swap)
		return float4(_mm_shuffle_ps(a.data, a.data, shuf_swap));
	else
		return a;
}

inline unsigned int to_bitmask(const float4& a) {
	return _mm_movemask_ps(a.data);
}

// Inverts a 4x4 matrix and returns the determinate
inline float invert_44_matrix(float* src) {
	// Code pulled from "Streaming SIMD Extensions - Inverse of 4x4 Matrix"
	// by Intel.
	// ftp://download.intel.com/design/PentiumIII/sml/24504301.pdf
	__m128 minor0;
	__m128 minor1;
	__m128 minor2;
	__m128 minor3;
	__m128 row0;
	__m128 row1;
	__m128 row2;
	__m128 row3;
	__m128 det;
	__m128 tmp1;
	tmp1 = _mm_loadh_pi(_mm_loadl_pi(tmp1, (__m64*)(src)), (__m64*)(src+ 4));
	row1 = _mm_loadh_pi(_mm_loadl_pi(row1, (__m64*)(src+8)), (__m64*)(src+12));
	row0 = _mm_shuffle_ps(tmp1, row1, 0x88);
	row1 = _mm_shuffle_ps(row1, tmp1, 0xDD);
	tmp1 = _mm_loadh_pi(_mm_loadl_pi(tmp1, (__m64*)(src+ 2)), (__m64*)(src+ 6));
	row3 = _mm_loadh_pi(_mm_loadl_pi(row3, (__m64*)(src+10)), (__m64*)(src+14));
	row2 = _mm_shuffle_ps(tmp1, row3, 0x88);
	row3 = _mm_shuffle_ps(row3, tmp1, 0xDD);
	// -----------------------------------------------
	tmp1 = _mm_mul_ps(row2, row3);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
	minor0 = _mm_mul_ps(row1, tmp1);
	minor1 = _mm_mul_ps(row0, tmp1);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
	minor0 = _mm_sub_ps(_mm_mul_ps(row1, tmp1), minor0);
	minor1 = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor1);
	minor1 = _mm_shuffle_ps(minor1, minor1, 0x4E);
	// -----------------------------------------------
	tmp1 = _mm_mul_ps(row1, row2);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
	minor0 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor0);
	minor3 = _mm_mul_ps(row0, tmp1);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
	minor0 = _mm_sub_ps(minor0, _mm_mul_ps(row3, tmp1));
	minor3 = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor3);
	minor3 = _mm_shuffle_ps(minor3, minor3, 0x4E);
	// -----------------------------------------------
	tmp1 = _mm_mul_ps(_mm_shuffle_ps(row1, row1, 0x4E), row3);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
	row2 = _mm_shuffle_ps(row2, row2, 0x4E);
	minor0 = _mm_add_ps(_mm_mul_ps(row2, tmp1), minor0);
	minor2 = _mm_mul_ps(row0, tmp1);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
	minor0 = _mm_sub_ps(minor0, _mm_mul_ps(row2, tmp1));
	minor2 = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor2);
	minor2 = _mm_shuffle_ps(minor2, minor2, 0x4E);
	// -----------------------------------------------
	tmp1 = _mm_mul_ps(row0, row1);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
	minor2 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor2);
	minor3 = _mm_sub_ps(_mm_mul_ps(row2, tmp1), minor3);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
	minor2 = _mm_sub_ps(_mm_mul_ps(row3, tmp1), minor2);
	minor3 = _mm_sub_ps(minor3, _mm_mul_ps(row2, tmp1));
	// -----------------------------------------------
	tmp1 = _mm_mul_ps(row0, row3);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
	minor1 = _mm_sub_ps(minor1, _mm_mul_ps(row2, tmp1));
	minor2 = _mm_add_ps(_mm_mul_ps(row1, tmp1), minor2);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
	minor1 = _mm_add_ps(_mm_mul_ps(row2, tmp1), minor1);
	minor2 = _mm_sub_ps(minor2, _mm_mul_ps(row1, tmp1));
	// -----------------------------------------------
	tmp1 = _mm_mul_ps(row0, row2);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
	minor1 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor1);
	minor3 = _mm_sub_ps(minor3, _mm_mul_ps(row1, tmp1));
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
	minor1 = _mm_sub_ps(minor1, _mm_mul_ps(row3, tmp1));
	minor3 = _mm_add_ps(_mm_mul_ps(row1, tmp1), minor3);
	// -----------------------------------------------
	det = _mm_mul_ps(row0, minor0);
	det = _mm_add_ps(_mm_shuffle_ps(det, det, 0x4E), det);
	det = _mm_add_ss(_mm_shuffle_ps(det, det, 0xB1), det);
	tmp1 = _mm_rcp_ss(det);
	det = _mm_sub_ss(_mm_add_ss(tmp1, tmp1), _mm_mul_ss(det, _mm_mul_ss(tmp1, tmp1)));
	det = _mm_shuffle_ps(det, det, 0x00);
	minor0 = _mm_mul_ps(det, minor0);
	_mm_storel_pi((__m64*)(src), minor0);
	_mm_storeh_pi((__m64*)(src+2), minor0);
	minor1 = _mm_mul_ps(det, minor1);
	_mm_storel_pi((__m64*)(src+4), minor1);
	_mm_storeh_pi((__m64*)(src+6), minor1);
	minor2 = _mm_mul_ps(det, minor2);
	_mm_storel_pi((__m64*)(src+ 8), minor2);
	_mm_storeh_pi((__m64*)(src+10), minor2);
	minor3 = _mm_mul_ps(det, minor3);
	_mm_storel_pi((__m64*)(src+12), minor3);
	_mm_storeh_pi((__m64*)(src+14), minor3);

	return det[0];
}

}

#endif // SIMD_HPP