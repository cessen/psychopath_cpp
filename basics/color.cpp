#include "numtype.h"
#include <assert.h>

#include "color.hpp"

Color Color::operator+(Color &b)
{
	Color col;
	for (int i=0; i < SPECTRUM_COUNT; i++) {
		col.spectrum[i] = spectrum[i] + b.spectrum[i];
	}

	return col;
}

Color Color::operator-(Color &b)
{
	Color col;
	for (int i=0; i < SPECTRUM_COUNT; i++) {
		col.spectrum[i] = spectrum[i] - b.spectrum[i];
	}

	return col;
}

Color Color::operator*(Color &b)
{
	Color col;
	for (int i=0; i < SPECTRUM_COUNT; i++) {
		col.spectrum[i] = spectrum[i] * b.spectrum[i];
	}

	return col;
}

Color Color::operator*(float32 b)
{
	Color col;
	for (int i=0; i < SPECTRUM_COUNT; i++) {
		col.spectrum[i] = spectrum[i] * b;
	}

	return col;
}

Color Color::operator/(float32 b)
{
	assert(b != 0.0f);
	Color col;
	for (int i=0; i < SPECTRUM_COUNT; i++) {
		col.spectrum[i] = spectrum[i] / b;
	}

	return col;
}

