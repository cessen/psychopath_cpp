#ifndef SAMPLE_HPP
#define SAMPLE_HPP

#include "numtype.h"

struct Sample {
	float32 x, y; // Image plane coordinates
	float32 u, v; // Lens coordinates
	float32 t; // Time coordinate
};

#endif
