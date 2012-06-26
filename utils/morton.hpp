/*
 * Morton code (a.k.a. z-curve) transforms.
 */

#ifndef MORTON_HPP
#define MORTON_HPP

namespace Morton
{

/**
 * @brief Encodes x and y coordinates into a morton code index.
 *
 * In practice x and y need to be within the range of an unsigned 16 bit
 * integer, since the output is a single 32 bit index.
 */
static inline uint32 xy2d(uint32 x, uint32 y)
{
	x &= 0x0000ffff;
	y &= 0x0000ffff;
	x |= (x << 8);
	y |= (y << 8);
	x &= 0x00ff00ff;
	y &= 0x00ff00ff;
	x |= (x << 4);
	y |= (y << 4);
	x &= 0x0f0f0f0f;
	y &= 0x0f0f0f0f;
	x |= (x << 2);
	y |= (y << 2);
	x &= 0x33333333;
	y &= 0x33333333;
	x |= (x << 1);
	y |= (y << 1);
	x &= 0x55555555;
	y &= 0x55555555;
	return x | (y << 1);
}

/**
 * @brief Decodes a morton code index into x and y coordinates.
 */
static inline void d2xy(uint32 d, uint32 *x, uint32 *y)
{
	*x = d;
	*y = (*x >> 1);
	*x &= 0x55555555;
	*y &= 0x55555555;
	*x |= (*x >> 1);
	*y |= (*y >> 1);
	*x &= 0x33333333;
	*y &= 0x33333333;
	*x |= (*x >> 2);
	*y |= (*y >> 2);
	*x &= 0x0f0f0f0f;
	*y &= 0x0f0f0f0f;
	*x |= (*x >> 4);
	*y |= (*y >> 4);
	*x &= 0x00ff00ff;
	*y &= 0x00ff00ff;
	*x |= (*x >> 8);
	*y |= (*y >> 8);
	*x &= 0x0000ffff;
	*y &= 0x0000ffff;
}

}

#endif // MORTON_HPP
