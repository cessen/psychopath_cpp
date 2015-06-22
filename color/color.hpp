#ifndef COLOR_HPP
#define COLOR_HPP

#include "numtype.h"
#include <cassert>
#include <cmath>
#include <array>
#include <tuple>

#include "utils.hpp"

#include "spectrum_grid.h"


/**
 * A single spectral sample.
 *
 * Wavelengths should be between 390 - 700 nm.
 */
struct SpectralSample {
	float wavelength; // Wavelength in nm
	float i; // Intensity

	SpectralSample operator+(const SpectralSample &other) const {
		SpectralSample s;
		s.wavelength = wavelength;
		s.i = i + other.i;
		return s;
	}
	SpectralSample& operator+=(const SpectralSample &other) {
		i += other.i;
		return *this;
	}

	SpectralSample operator*(const SpectralSample &other) const {
		SpectralSample s;
		s.wavelength = wavelength;
		s.i = i * other.i;
		return s;
	}
	SpectralSample& operator*=(const SpectralSample &other) {
		i *= other.i;
		return *this;
	}

	SpectralSample operator*(float n) const {
		SpectralSample s;
		s.wavelength = wavelength;
		s.i = i * n;
		return s;
	}
	SpectralSample& operator*=(float n) {
		i *= n;
		return *this;
	}

	SpectralSample operator/(float n) const {
		SpectralSample s;
		s.wavelength = wavelength;
		s.i = i / n;
		return s;
	}
	SpectralSample& operator/=(float n) {
		i /= n;
		return *this;
	}
};


/**
 * Close analytic approximations of the CIE 1931 XYZ color curves.
 * From the paper "Simple Analytic Approximations to the CIE XYZ Color Matching
 * Functions" by Wyman et al.
 *
 * @param wavelength The wavelength of light in nm.
 * @returns The sensitivity of the curve at that wavelength.
 */
static inline float X_1931(float wavelength)
{
	float t1 = (wavelength - 442.0f) * ((wavelength < 442.0f) ? 0.0624f : 0.0374f);
	float t2 = (wavelength - 599.8f) * ((wavelength < 599.8f) ? 0.0264f : 0.0323f);
	float t3 = (wavelength - 501.1f) * ((wavelength < 501.1f) ? 0.0490f : 0.0382f);
	return (0.362f * std::exp(-0.5f * t1 * t1)) + (1.056f * std::exp(-0.5f * t2 * t2)) - (0.065f * std::exp(-0.5f * t3 * t3));
}
static inline float Y_1931(float wavelength)
{
	float t1 = (wavelength - 568.8f) * ((wavelength < 568.8f) ? 0.0213f : 0.0247f);
	float t2 = (wavelength - 530.9f) * ((wavelength < 530.9f) ? 0.0613f : 0.0322f);
	return (0.821f * std::exp(-0.5f * t1 * t1)) + (0.286f * std::exp(-0.5f * t2 * t2));
}
static inline float Z_1931(float wavelength)
{
	float t1 = (wavelength - 437.0f) * ((wavelength < 437.0f) ? 0.0845f : 0.0278f);
	float t2 = (wavelength - 459.0f) * ((wavelength < 459.0f) ? 0.0385f : 0.0725f);
	return (1.217f * std::exp(-0.5f * t1 * t1)) + (0.681f * std::exp(-0.5f * t2 * t2));
}


/**
 * A color represented in CIE 1931 XYZ color space.
 *
 * This is primarily used for accumulating spectral color samples.
 */
struct Color_XYZ {
	float x, y, z;

	Color_XYZ() = default;
	Color_XYZ(float intensity): x {intensity}, y {intensity}, z {intensity} {}
	Color_XYZ(float intensity, float wavelength): x {X_1931(wavelength)*intensity}, y {Y_1931(wavelength)*intensity}, z {Z_1931(wavelength)*intensity} {}
	Color_XYZ(SpectralSample s): x {X_1931(s.wavelength)*s.i}, y {Y_1931(s.wavelength)*s.i}, z {Z_1931(s.wavelength)*s.i} {}
	Color_XYZ(float x, float y, float z): x {x}, y {y}, z {z} {}

	float &operator[](int i) {
		assert(i >= 0 && i < 3);
		return (&x)[i];
	}

	const float &operator[](int i) const {
		assert(i >= 0 && i < 3);
		return (&x)[i];
	}

	Color_XYZ operator+(const Color_XYZ &b) const {
		return Color_XYZ(x + b.x, y + b.y, z + b.z);
	}
	Color_XYZ& operator+=(const Color_XYZ &b) {
		x += b.x;
		y += b.y;
		z += b.z;
		return *this;
	}

	Color_XYZ operator-(const Color_XYZ &b) const {
		return Color_XYZ(x + b.x, y + b.y, z + b.z);
	}
	Color_XYZ& operator-=(const Color_XYZ &b) {
		x += b.x;
		y += b.y;
		z += b.z;
		return *this;
	}

	Color_XYZ operator*(float n) const {
		return Color_XYZ(x * n, y * n, z * n);
	}
	Color_XYZ& operator*=(float n) {
		x *= n;
		y *= n;
		z *= n;
		return *this;
	}

	Color_XYZ operator/(float n) const {
		return Color_XYZ(x / n, y / n, z / n);
	}
	Color_XYZ& operator/=(float n) {
		x /= n;
		y /= n;
		z /= n;
		return *this;
	}

	void add_light(float intensity, float wavelength) {
		x += X_1931(wavelength) * intensity;
		y += Y_1931(wavelength) * intensity;
		z += Z_1931(wavelength) * intensity;
	}

	void add_light(SpectralSample s) {
		x += X_1931(s.wavelength) * s.i;
		y += Y_1931(s.wavelength) * s.i;
		z += Z_1931(s.wavelength) * s.i;
	}
};




struct Color {
	std::array<float, 3> spectrum;

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

	Color& operator+=(const Color &b) {
		for (unsigned int i=0; i < spectrum.size(); i++) {
			spectrum[i] += b.spectrum[i];
		}

		return *this;
	}

	Color operator-(const Color &b) const {
		Color col;
		for (unsigned int i=0; i < spectrum.size(); i++) {
			col.spectrum[i] = spectrum[i] - b.spectrum[i];
		}

		return col;
	}

	Color& operator-=(const Color &b) {
		for (unsigned int i=0; i < spectrum.size(); i++) {
			spectrum[i] -= b.spectrum[i];
		}

		return *this;
	}

	Color operator*(const Color &b) const {
		Color col;
		for (unsigned int i=0; i < spectrum.size(); i++) {
			col.spectrum[i] = spectrum[i] * b.spectrum[i];
		}

		return col;
	}

	Color& operator*=(const Color &b) {
		for (unsigned int i=0; i < spectrum.size(); i++) {
			spectrum[i] *= b.spectrum[i];
		}

		return *this;
	}

	Color operator*(const float b) const {
		Color col;
		for (unsigned int i=0; i < spectrum.size(); i++) {
			col.spectrum[i] = spectrum[i] * b;
		}

		return col;
	}

	Color& operator*=(const float b) {
		for (unsigned int i=0; i < spectrum.size(); i++) {
			spectrum[i] *= b;
		}

		return *this;
	}

	Color operator/(const float b) const {
		assert(b != 0.0f);
		Color col;
		for (unsigned int i=0; i < spectrum.size(); i++) {
			col.spectrum[i] = spectrum[i] / b;
		}

		return col;
	}

	Color& operator/=(const float b) {
		for (unsigned int i=0; i < spectrum.size(); i++) {
			spectrum[i] /= b;
		}

		return *this;
	}

	float energy() const {
		float accum = 0.0f;
		for (auto sp: spectrum)
			accum += sp;
		accum /= spectrum.size();
		return accum;
	}
};




/********************************
 * sRGB/XYZ conversion functions
 ********************************/
static inline float sRGB_gamma(float n)
{
	return n < 0.0031308f ? (n * 12.92f) : ((1.055f * std::pow(n, 1.0f/2.4f)) - 0.055f);
}

static inline float sRGB_inv_gamma(float n)
{
	return n < 0.04045f ? (n / 12.92f) : std::pow(((n + 0.055f) / 1.055f), 2.4f);
}

static inline std::tuple<float, float, float> XYZ_to_sRGB(Color_XYZ col)
{
	std::tuple<float, float, float> srgb;

	// First convert from XYZ to linear sRGB
	std::get<0>(srgb) = clamp((col.x *  3.2406f) + (col.y * -1.5372f) + (col.z * -0.4986f), 0.0f, 1.0f);
	std::get<1>(srgb) = clamp((col.x * -0.9689f) + (col.y *  1.8758f) + (col.z *  0.0415f), 0.0f, 1.0f);
	std::get<2>(srgb) = clamp((col.x *  0.0557f) + (col.y * -0.2040f) + (col.z *  1.0570f), 0.0f, 1.0f);

	// Then "gamma" correct
	std::get<0>(srgb) = sRGB_gamma(std::get<0>(srgb));
	std::get<1>(srgb) = sRGB_gamma(std::get<1>(srgb));
	std::get<2>(srgb) = sRGB_gamma(std::get<2>(srgb));

	return srgb;
}

static inline Color_XYZ sRGB_to_XYZ(std::tuple<float, float, float> col)
{
	Color_XYZ xyz;

	// Undo "gamma" correction
	std::get<0>(col) = sRGB_inv_gamma(std::get<0>(col));
	std::get<1>(col) = sRGB_inv_gamma(std::get<1>(col));
	std::get<2>(col) = sRGB_inv_gamma(std::get<2>(col));

	// Convert from linear sRGB to XYZ
	xyz.x = (std::get<0>(col) * 0.4124f) + (std::get<1>(col) * 0.3576f) + (std::get<2>(col) * 0.1805f);
	xyz.y = (std::get<0>(col) * 0.2126f) + (std::get<1>(col) * 0.7152f) + (std::get<2>(col) * 0.0722f);
	xyz.z = (std::get<0>(col) * 0.0193f) + (std::get<1>(col) * 0.1192f) + (std::get<2>(col) * 0.9505f);

	return xyz;
}

// Treat Color as linear sRGB
static inline Color_XYZ Color_to_XYZ(Color col)
{
	Color_XYZ xyz;

	// Convert from linear sRGB to XYZ
	xyz.x = (col[0] * 0.4124f) + (col[1] * 0.3576f) + (col[2] * 0.1805f);
	xyz.y = (col[0] * 0.2126f) + (col[1] * 0.7152f) + (col[2] * 0.0722f);
	xyz.z = (col[0] * 0.0193f) + (col[1] * 0.1192f) + (col[2] * 0.9505f);

	return xyz;
}


/*************************************************************************
 * Functions for evaluating various color representations at
 * spectral wavelengths.
 *
 * The approach taken to upsample colors to spectrum is from the paper
 * "Physically Meaningful Rendering using Tristimulus Colours" by Hanika et al.
 *************************************************************************/
static inline float XYZ_to_spectrum(const Color_XYZ& col, float wavelength)
{
	// TODO: figure out the correct equal_energy_reflectance factor given
	// the maths elsewhere in psychopath.
	return spectrum_xyz_to_p(wavelength, &(col.x)) / equal_energy_reflectance * M_PI * 0.5f;
}

static inline float Color_to_spectrum(const Color& col, float wavelength)
{
	const auto tmp = Color_to_XYZ(col);
	return spectrum_xyz_to_p(wavelength, &(tmp.x)) / equal_energy_reflectance * M_PI * 0.5f;
}


#endif
