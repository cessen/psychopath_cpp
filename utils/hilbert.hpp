/* Hilbert curve transforms.
 */

#ifndef HILBERT_HPP
#define HILBERT_HPP

#include "numtype.h"

namespace Hilbert
{

// Utility function used by the functions below.
static inline void hil_rot(uint32_t n, uint32_t &x, uint32_t &y, uint32_t rx, uint32_t ry)
{
	if (ry == 0) {
		if (rx == 1) {
			x = n-1 - x;
			y = n-1 - y;
		}
		const uint32_t t = x;
		x = y;
		y = t;
	}
}

/**
 * @brief Convert (x,y) to hilbert curve index.
 *
 * @param x The x coordinate.  Must be a positive integer no greater than n.
 * @param y The y coordinate.  Must be a positive integer no greater than n.
 *
 * @returns The hilbert curve index corresponding to the (x,y) coordinates given.
 */
static inline uint32_t xy2d(uint32_t x, uint32_t y)
{
	const uint32_t n = 1 << 16;
	uint32_t rx, ry, s, d=0;
	for (s=n>>1; s>0; s>>=1) {
		rx = (x & s) > 0;
		ry = (y & s) > 0;
		d += s * s * ((3 * rx) ^ ry);
		hil_rot(s, x, y, rx, ry);
	}
	return d;
}


/**
 * @brief Convert hilbert curve index to (x,y).
 *
 * @param d The hilbert curve index.
 * @param[out] x Pointer where the x coordinate will be stored.
 * @param[out] y Pointer where the y coordinate will be stored.
 */
static inline void d2xy(uint32_t d, uint32_t *x, uint32_t *y)
{
	const uint32_t n = 1 << 16;
	uint32_t rx, ry, s, t=d;
	*x = *y = 0;
	for (s=1; s<n; s<<=1) {
		rx = 1 & (t>>1);
		ry = 1 & (t ^ rx);
		hil_rot(s, *x, *y, rx, ry);
		*x += s * rx;
		*y += s * ry;
		t >>= 2;
	}
}

}

#endif // HILBERT_HPP


