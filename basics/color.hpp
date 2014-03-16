#ifndef COLOR_HPP
#define COLOR_HPP

#include "numtype.h"
#include <array>

#define SPECTRUM_COUNT 3

struct Color {
	std::array<float, SPECTRUM_COUNT> spectrum;

	Color(float n=0.0) {

		for (auto& sp: spectrum) {
			sp = n;
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
		for (unsigned int i=0; i < spectrum.size(); i++) {
			col.spectrum[i] = spectrum[i] + b.spectrum[i];
		}

		return col;
	}

	void operator+=(const Color &b) {
		for (unsigned int i=0; i < spectrum.size(); i++) {
			spectrum[i] += b.spectrum[i];
		}
	}

	Color operator-(const Color &b) const {
		Color col;
		for (unsigned int i=0; i < spectrum.size(); i++) {
			col.spectrum[i] = spectrum[i] - b.spectrum[i];
		}

		return col;
	}

	void operator-=(const Color &b) {
		for (unsigned int i=0; i < spectrum.size(); i++) {
			spectrum[i] -= b.spectrum[i];
		}
	}

	Color operator*(const Color &b) const {
		Color col;
		for (unsigned int i=0; i < spectrum.size(); i++) {
			col.spectrum[i] = spectrum[i] * b.spectrum[i];
		}

		return col;
	}

	void operator*=(const Color &b) {
		for (unsigned int i=0; i < spectrum.size(); i++) {
			spectrum[i] *= b.spectrum[i];
		}
	}

	Color operator*(const float b) const {
		Color col;
		for (unsigned int i=0; i < spectrum.size(); i++) {
			col.spectrum[i] = spectrum[i] * b;
		}

		return col;
	}

	void operator*=(const float b) {
		for (unsigned int i=0; i < spectrum.size(); i++) {
			spectrum[i] *= b;
		}
	}

	Color operator/(const float b) const {
		assert(b != 0.0f);
		Color col;
		for (unsigned int i=0; i < spectrum.size(); i++) {
			col.spectrum[i] = spectrum[i] / b;
		}

		return col;
	}

	void operator/=(const float b) {
		for (unsigned int i=0; i < spectrum.size(); i++) {
			spectrum[i] /= b;
		}
	}

	float energy() const {
		float accum = 0.0f;
		for (auto sp: spectrum)
			accum += sp;
		accum /= spectrum.size();
		return accum;
	}
};

#endif
