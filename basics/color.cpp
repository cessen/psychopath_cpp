#include "numtype.h"
#include <assert.h>

#include "color.hpp"

Color Color::operator+(const Color &b) const
{
	Color col;
	for (int i=0; i < SPECTRUM_COUNT; i++) {
		col.spectrum[i] = spectrum[i] + b.spectrum[i];
	}

	return col;
}

Color Color::operator-(const Color &b) const
{
	Color col;
	for (int i=0; i < SPECTRUM_COUNT; i++) {
		col.spectrum[i] = spectrum[i] - b.spectrum[i];
	}

	return col;
}

Color Color::operator*(const Color &b) const
{
	Color col;
	for (int i=0; i < SPECTRUM_COUNT; i++) {
		col.spectrum[i] = spectrum[i] * b.spectrum[i];
	}

	return col;
}

Color Color::operator*(const float32 b) const
{
	Color col;
	for (int i=0; i < SPECTRUM_COUNT; i++) {
		col.spectrum[i] = spectrum[i] * b;
	}

	return col;
}

Color Color::operator/(const float32 b) const
{
	assert(b != 0.0f);
	Color col;
	for (int i=0; i < SPECTRUM_COUNT; i++) {
		col.spectrum[i] = spectrum[i] / b;
	}

	return col;
}

