#ifndef COLOR_HPP
#define COLOR_HPP

#include "numtype.h"
#include <cassert>
#include <cmath>
#include <array>
#include <tuple>

#include "utils.hpp"

#include "spectrum_grid.h"

// Min and max wavelength used in sampling the spectrum
static constexpr float WAVELENGTH_MIN = 380.0f;
static constexpr float WAVELENGTH_MAX = 700.0f;
static constexpr float WAVELENGTH_RANGE = WAVELENGTH_MAX - WAVELENGTH_MIN;

// 1 over the integral of any of the XYZ curves
static constexpr float INV_XYZ_INTEGRAL = 0.009358239977091027f;

// Normalizing factor for when accumulating XYZ color
static constexpr float XYZ_NORM_FAC = INV_XYZ_INTEGRAL * (WAVELENGTH_MAX - WAVELENGTH_MIN);

#define SPECTRAL_COUNT 4

/**
 * Gets the nth wavelength given a hero wavelength, as per the paper
 * "Hero Wavelength Spectral Sampling" by Wilkie et al.
 */
static inline float wavelength_n(float hero_wavelength, int n) {
	assert(n < SPECTRAL_COUNT);
	hero_wavelength += n * (WAVELENGTH_RANGE / SPECTRAL_COUNT);
	if (hero_wavelength > WAVELENGTH_MAX) {
		hero_wavelength -= WAVELENGTH_RANGE;
	}
	return hero_wavelength;
}

/**
 * A spectral sample.
 *
 * Contains N actual spectral samples, distributed evenly over the visible
 * spectrum based on the given wavelength, as per the paper "Hero Wavelength
 * Spectral Sampling" by Wilkie et al.
 */
struct SpectralSample {
	float e[SPECTRAL_COUNT]; // Energies at the various wavelengths
	float hero_wavelength; // Wavelength in nm

	// Constructors
	SpectralSample() = default;
	explicit SpectralSample(float w): hero_wavelength {w} {}
	explicit SpectralSample(float w, float n): hero_wavelength {w} {
		for (int i = 0; i < SPECTRAL_COUNT; ++i) {
			e[i] = n;
		}
	}

	// Misc convenience functions
	void set_all_e(float n) {
		for (int i = 0; i < SPECTRAL_COUNT; ++i) {
			e[i] = n;
		}
	}

	float wavelength_n(int i) const {
		return ::wavelength_n(hero_wavelength, i);
	}

	float sum_wavelength_energy() const {
		float sum = 0.0f;
		for (int i = 0; i < SPECTRAL_COUNT; ++i) {
			sum += e[i];
		}
		return sum;
	}

	// Math functions
	SpectralSample operator+(const SpectralSample &other) const {
		SpectralSample s;
		s.hero_wavelength = hero_wavelength;
		for (int i = 0; i < SPECTRAL_COUNT; ++i) {
			s.e[i] = e[i] + other.e[i];
		}
		return s;
	}
	SpectralSample& operator+=(const SpectralSample &other) {
		for (int i = 0; i < SPECTRAL_COUNT; ++i) {
			e[i] += other.e[i];
		}
		return *this;
	}

	SpectralSample operator*(const SpectralSample &other) const {
		SpectralSample s;
		s.hero_wavelength = hero_wavelength;
		for (int i = 0; i < SPECTRAL_COUNT; ++i) {
			s.e[i] = e[i] * other.e[i];
		}
		return s;
	}
	SpectralSample& operator*=(const SpectralSample &other) {
		for (int i = 0; i < SPECTRAL_COUNT; ++i) {
			e[i] *= other.e[i];
		}
		return *this;
	}

	SpectralSample operator*(float n) const {
		SpectralSample s;
		s.hero_wavelength = hero_wavelength;
		for (int i = 0; i < SPECTRAL_COUNT; ++i) {
			s.e[i] = e[i] * n;
		}
		return s;
	}
	SpectralSample& operator*=(float n) {
		for (int i = 0; i < SPECTRAL_COUNT; ++i) {
			e[i] *= n;
		}
		return *this;
	}

	SpectralSample operator/(float n) const {
		SpectralSample s;
		s.hero_wavelength = hero_wavelength;
		for (int i = 0; i < SPECTRAL_COUNT; ++i) {
			s.e[i] = e[i] / n;
		}
		return s;
	}
	SpectralSample& operator/=(float n) {
		for (int i = 0; i < SPECTRAL_COUNT; ++i) {
			e[i] /= n;
		}
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
static inline float X_1931(float wavelength) {
	float t1 = (wavelength - 442.0f) * ((wavelength < 442.0f) ? 0.0624f : 0.0374f);
	float t2 = (wavelength - 599.8f) * ((wavelength < 599.8f) ? 0.0264f : 0.0323f);
	float t3 = (wavelength - 501.1f) * ((wavelength < 501.1f) ? 0.0490f : 0.0382f);
	return (0.362f * std::exp(-0.5f * t1 * t1)) + (1.056f * std::exp(-0.5f * t2 * t2)) - (0.065f * std::exp(-0.5f * t3 * t3));
}
static inline float Y_1931(float wavelength) {
	float t1 = (wavelength - 568.8f) * ((wavelength < 568.8f) ? 0.0213f : 0.0247f);
	float t2 = (wavelength - 530.9f) * ((wavelength < 530.9f) ? 0.0613f : 0.0322f);
	return (0.821f * std::exp(-0.5f * t1 * t1)) + (0.286f * std::exp(-0.5f * t2 * t2));
}
static inline float Z_1931(float wavelength) {
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
	explicit Color_XYZ(float intensity): x {intensity}, y {intensity}, z {intensity} {}
	explicit Color_XYZ(float intensity, float wavelength): x {X_1931(wavelength)*intensity}, y {Y_1931(wavelength)*intensity}, z {Z_1931(wavelength)*intensity} {}
	explicit Color_XYZ(SpectralSample s): x {0.0f}, y {0.0f}, z {0.0f} {
		for (int i = 0; i < SPECTRAL_COUNT; ++i) {
			x += X_1931(s.wavelength_n(i)) * s.e[i];
			y += Y_1931(s.wavelength_n(i)) * s.e[i];
			z += Z_1931(s.wavelength_n(i)) * s.e[i];
		}
		x *= 1.0f / SPECTRAL_COUNT;
		y *= 1.0f / SPECTRAL_COUNT;
		z *= 1.0f / SPECTRAL_COUNT;
	}
	explicit Color_XYZ(float x, float y, float z): x {x}, y {y}, z {z} {}

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
		*this += Color_XYZ(s);
	}
};



// An RGB specified color.
// This is assumed to be the same as linear sRGB, except scaled to have a
// white point at rgb<1,1,1> instead of D65.
struct Color {
	std::array<float, 3> spectrum;

	Color(): spectrum {0.0f, 0.0f, 0.0f} {}

	explicit Color(float n) {

		for (auto& sp: spectrum) {
			sp = n;
		}
	}

	explicit Color(float r_, float g_, float b_) {
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
 * Colorspace conversion functions
 ********************************/
static inline float sRGB_gamma(float n) {
	return n < 0.0031308f ? (n * 12.92f) : ((1.055f * std::pow(n, 1.0f/2.4f)) - 0.055f);
}

static inline float sRGB_inv_gamma(float n) {
	return n < 0.04045f ? (n / 12.92f) : std::pow(((n + 0.055f) / 1.055f), 2.4f);
}

static inline std::tuple<float, float, float> XYZ_to_sRGB(Color_XYZ xyz) {
	std::tuple<float, float, float> srgb;

	// First convert from XYZ to linear sRGB
	std::get<0>(srgb) = clamp((xyz.x *  3.2406f) + (xyz.y * -1.5372f) + (xyz.z * -0.4986f), 0.0f, 1.0f);
	std::get<1>(srgb) = clamp((xyz.x * -0.9689f) + (xyz.y *  1.8758f) + (xyz.z *  0.0415f), 0.0f, 1.0f);
	std::get<2>(srgb) = clamp((xyz.x *  0.0557f) + (xyz.y * -0.2040f) + (xyz.z *  1.0570f), 0.0f, 1.0f);

	// Then "gamma" correct
	std::get<0>(srgb) = sRGB_gamma(std::get<0>(srgb));
	std::get<1>(srgb) = sRGB_gamma(std::get<1>(srgb));
	std::get<2>(srgb) = sRGB_gamma(std::get<2>(srgb));

	return srgb;
}

static inline Color_XYZ sRGB_to_XYZ(std::tuple<float, float, float> srgb) {
	Color_XYZ xyz;

	// Undo "gamma" correction
	std::get<0>(srgb) = sRGB_inv_gamma(std::get<0>(srgb));
	std::get<1>(srgb) = sRGB_inv_gamma(std::get<1>(srgb));
	std::get<2>(srgb) = sRGB_inv_gamma(std::get<2>(srgb));

	// Convert from linear sRGB to XYZ
	xyz.x = (std::get<0>(srgb) * 0.4124f) + (std::get<1>(srgb) * 0.3576f) + (std::get<2>(srgb) * 0.1805f);
	xyz.y = (std::get<0>(srgb) * 0.2126f) + (std::get<1>(srgb) * 0.7152f) + (std::get<2>(srgb) * 0.0722f);
	xyz.z = (std::get<0>(srgb) * 0.0193f) + (std::get<1>(srgb) * 0.1192f) + (std::get<2>(srgb) * 0.9505f);

	return xyz;
}

// Conversion for sRGB scaled to have whitepoint E
static inline std::tuple<float, float, float> XYZ_to_sRGB_E(Color_XYZ xyz) {
	std::tuple<float, float, float> srgbe;

	// First convert from XYZ to linear sRGB with whitepoint E
	std::get<0>(srgbe) = clamp((xyz.x *  2.6897f) + (xyz.y * -1.2759f) + (xyz.z * -0.4138f), 0.0f, 1.0f);
	std::get<1>(srgbe) = clamp((xyz.x * -1.0216f) + (xyz.y *  1.9778f) + (xyz.z *  0.0438f), 0.0f, 1.0f);
	std::get<2>(srgbe) = clamp((xyz.x *  0.0613f) + (xyz.y * -0.2245f) + (xyz.z *  1.1632f), 0.0f, 1.0f);

	// Then "gamma" correct
	std::get<0>(srgbe) = sRGB_gamma(std::get<0>(srgbe));
	std::get<1>(srgbe) = sRGB_gamma(std::get<1>(srgbe));
	std::get<2>(srgbe) = sRGB_gamma(std::get<2>(srgbe));

	return srgbe;
}

// Conversion for sRGB scaled to have whitepoint E
static inline Color_XYZ sRGB_E_to_XYZ(std::tuple<float, float, float> srgbe) {
	Color_XYZ xyz;

	// Undo "gamma" correction
	std::get<0>(srgbe) = sRGB_inv_gamma(std::get<0>(srgbe));
	std::get<1>(srgbe) = sRGB_inv_gamma(std::get<1>(srgbe));
	std::get<2>(srgbe) = sRGB_inv_gamma(std::get<2>(srgbe));

	// Convert from linear sRGB with whitepoint E to XYZ
	xyz.x = (std::get<0>(srgbe) * 0.4339f) + (std::get<1>(srgbe) * 0.3762f) + (std::get<2>(srgbe) * 0.1899f);
	xyz.y = (std::get<0>(srgbe) * 0.2126f) + (std::get<1>(srgbe) * 0.7152f) + (std::get<2>(srgbe) * 0.0722f);
	xyz.z = (std::get<0>(srgbe) * 0.0177f) + (std::get<1>(srgbe) * 0.1095f) + (std::get<2>(srgbe) * 0.8728f);

	return xyz;
}

static inline Color XYZ_to_Color(Color_XYZ xyz) {
	Color col;

	// Convert from XYZ to linear sRGB scaled to have a white point
	// at rgb<1,1,1>
	col[0] = std::max(0.0f, (xyz.x *  2.6897f) + (xyz.y * -1.2759f) + (xyz.z * -0.4138f));
	col[1] = std::max(0.0f, (xyz.x * -1.0216f) + (xyz.y *  1.9778f) + (xyz.z *  0.0438f));
	col[2] = std::max(0.0f, (xyz.x *  0.0613f) + (xyz.y * -0.2245f) + (xyz.z *  1.1632f));

	return col;
}

static inline Color_XYZ Color_to_XYZ(Color col) {
	Color_XYZ xyz;

	// Convert from linear sRGB scaled to have a white point at rgb<1,1,1>
	// to XYZ
	xyz.x = (col[0] * 0.4339f) + (col[1] * 0.3762f) + (col[2] * 0.1899f);
	xyz.y = (col[0] * 0.2126f) + (col[1] * 0.7152f) + (col[2] * 0.0722f);
	xyz.z = (col[0] * 0.0177f) + (col[1] * 0.1095f) + (col[2] * 0.8728f);

	return xyz;
}


/*************************************************************************
 * Functions for evaluating various color representations at
 * spectral wavelengths.
 *
 * The approach taken to upsample colors to spectrum is from the paper
 * "Physically Meaningful Rendering using Tristimulus Colours" by Hanika et al.
 *************************************************************************/
static inline float XYZ_to_spectrum(const Color_XYZ& xyz, float wavelength) {
	return spectrum_xyz_to_p(wavelength, &(xyz.x)) * (1.0f / equal_energy_reflectance);
}

static inline float Color_to_spectrum(const Color& col, float wavelength) {
	return XYZ_to_spectrum(Color_to_XYZ(col), wavelength);
}

static inline SpectralSample XYZ_to_SpectralSample(const Color_XYZ& xyz, float wavelength) {
	SpectralSample s;
	s.hero_wavelength = wavelength;

	for (int i = 0; i < SPECTRAL_COUNT; ++i) {
		s.e[i] = XYZ_to_spectrum(xyz, s.wavelength_n(i));
	}

	return s;
}

static inline SpectralSample Color_to_SpectralSample(const Color& col, float wavelength) {
	return XYZ_to_SpectralSample(Color_to_XYZ(col), wavelength);
}


#endif
