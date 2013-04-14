#ifndef COLOR_HPP
#define COLOR_HPP

#include "numtype.h"

#define SPECTRUM_COUNT 3

struct Color {
	float spectrum[SPECTRUM_COUNT];

	Color(float n=0.0) {

		for (int32_t i=0; i < SPECTRUM_COUNT; i++) {
			spectrum[i] = n;
		}
	}

	Color(float r_, float g_, float b_) {
		spectrum[0] = r_;
		spectrum[1] = g_;
		spectrum[2] = b_;
	}

	float &operator[](int i) {
		return spectrum[i];
	}

	const float &operator[](int i) const {
		return spectrum[i];
	}

	Color operator+(const Color &b) const {
		Color col;
		for (int i=0; i < SPECTRUM_COUNT; i++) {
			col.spectrum[i] = spectrum[i] + b.spectrum[i];
		}

		return col;
	}

	void operator+=(const Color &b) {
		for (int i=0; i < SPECTRUM_COUNT; i++) {
			spectrum[i] += b.spectrum[i];
		}
	}

	Color operator-(const Color &b) const {
		Color col;
		for (int i=0; i < SPECTRUM_COUNT; i++) {
			col.spectrum[i] = spectrum[i] - b.spectrum[i];
		}

		return col;
	}

	void operator-=(const Color &b) {
		for (int i=0; i < SPECTRUM_COUNT; i++) {
			spectrum[i] -= b.spectrum[i];
		}
	}

	Color operator*(const Color &b) const {
		Color col;
		for (int i=0; i < SPECTRUM_COUNT; i++) {
			col.spectrum[i] = spectrum[i] * b.spectrum[i];
		}

		return col;
	}

	void operator*=(const Color &b) {
		for (int i=0; i < SPECTRUM_COUNT; i++) {
			spectrum[i] *= b.spectrum[i];
		}
	}

	Color operator*(const float b) const {
		Color col;
		for (int i=0; i < SPECTRUM_COUNT; i++) {
			col.spectrum[i] = spectrum[i] * b;
		}

		return col;
	}

	void operator*=(const float b) {
		for (int i=0; i < SPECTRUM_COUNT; i++) {
			spectrum[i] *= b;
		}
	}

	Color operator/(const float b) const {
		assert(b != 0.0f);
		Color col;
		for (int i=0; i < SPECTRUM_COUNT; i++) {
			col.spectrum[i] = spectrum[i] / b;
		}

		return col;
	}

	void operator/=(const float b) {
		for (int i=0; i < SPECTRUM_COUNT; i++) {
			spectrum[i] /= b;
		}
	}
};

#endif
