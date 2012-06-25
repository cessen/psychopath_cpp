/* Non-recursive hilbert curve transforms.
 * Code taken (nearly) verbatim from the Wikipedia page on hilbert curves.
 */

#ifndef HILBERT_HPP
#define HILBERT_HPP

#include "numtype.h"

namespace Hilbert
{

// Utility function used by the functions below.
static inline void hil_rot(uint32 n, uint32 &x, uint32 &y, uint32 rx, uint32 ry)
{
	if (ry == 0) {
		if (rx == 1) {
			x = n-1 - x;
			y = n-1 - y;
		}

		//Swap x and y
		const uint32 t = x;
		x = y;
		y = t;
	}
}

/**
 * @brief Convert (x,y) to hilbert curve index.
 *
 * @param n The resolution of the hilbert curve (e.g. n*n).  Internally rounded
 *          up to the nearest power of two.
 * @param x The x coordinate.  Must be a positive integer no greater than n.
 * @param y The y coordinate.  Must be a positive integer no greater than n.
 *
 * @returns The hilbert curve index corresponding to the (x,y) coordinates given.
 */
static inline uint32 xy2d(uint32 x, uint32 y)
{
	const uint32 n = 1 << 16;
	uint32 rx, ry, s, d=0;
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
 * @param n The resolution of the hilbert curve (e.g. n*n).  Internally rounded
 *          up to the nearest power of two.
 * @param d The hilbert curve index.
 * @param[out] x Pointer where the x coordinate will be stored.
 * @param[out] y Pointer where the y coordinate will be stored.
 */
static inline void d2xy(uint32 d, uint32 *x, uint32 *y)
{
	const uint32 n = 1 << 16;
	uint32 rx, ry, s, t=d;
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


