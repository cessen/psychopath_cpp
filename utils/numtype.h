/*
 * Defines basic numerical types for use throughout psychopath.
 * These should be used in place of int, float, long, etc. in order
 * to keep consistent bit-widths across platforms.  This file can then
 * be customized per-platform to get the correct corresponding types.
 */

#ifndef NUMTYPE_H
#define NUMTYPE_H

#include <stdlib.h>

typedef unsigned char      byte;

typedef char               int8;
typedef short              int16;
typedef int                int32;
typedef long long          int64;

typedef unsigned int       uint; // For when you don't care about width
typedef unsigned char      uint8;
typedef unsigned short     uint16;
typedef unsigned int       uint32;
typedef unsigned long long uint64;

typedef float              float32;
typedef double             float64;

/*
 * Some psychopath-specific types.
 */
typedef size_t uint_i; // For id's/indices of e.g. lists of objects, rays, etc.

#endif // NUMTYPE_H
