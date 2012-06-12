#ifndef COLOR_HPP
#define COLOR_HPP

#include "numtype.h"

#define SPECTRUM_COUNT 3

struct Color {
	float32 spectrum[SPECTRUM_COUNT];

	Color(float32 n=0.0) {

		for (int32 i=0; i < SPECTRUM_COUNT; i++) {
			spectrum[i] = n;
		}
	}

	Color(float32 r_, float32 g_, float32 b_) {
		spectrum[0] = r_;
		spectrum[1] = g_;
		spectrum[2] = b_;
	}

	Color operator+(const Color &b) const;
	Color operator-(const Color &b) const;
	Color operator*(const Color &b) const;
	Color operator*(const float32 b) const;
	Color operator/(const float32 b) const;
};

#endif
