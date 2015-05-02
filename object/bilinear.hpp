#ifndef BILINEAR_HPP
#define BILINEAR_HPP

#include "numtype.h"

#include <atomic>
#include <vector>
#include <array>
#include "stack.hpp"
#include "vector.hpp"
#include "grid.hpp"
#include "object.hpp"

/*
 * A bilinear patch.
 * Vertices arranged like this:
 *     u-->
 *   v1----v2
 * v  |    |
 * | v4----v3
 * \/
 */
class Bilinear final: public PatchSurface
{
public:
	std::vector<std::array<Vec3, 4>> verts;
	std::vector<BBox> bbox;

	Bilinear() {}
	Bilinear(Vec3 v1, Vec3 v2, Vec3 v3, Vec3 v4);
	virtual ~Bilinear() {}

	void finalize();

	void add_time_sample(Vec3 v1, Vec3 v2, Vec3 v3, Vec3 v4);

	virtual const std::vector<BBox> &bounds() const override;
	virtual Color total_emitted_color() const override {
		return Color(0.0f);
	}


	// For being traced by intersect_rays_with_patch() in tracer.cpp
	typedef std::array<Vec3, 4> store_type;

	__attribute__((always_inline))
	static float ulen(const std::array<Vec3, 4> &p) {
		return longest_axis(p[0] - p[1]);
	}

	__attribute__((always_inline))
	static float vlen(const std::array<Vec3, 4> &p) {
		return longest_axis(p[0] - p[3]);
	}

	__attribute__((always_inline))
	static void split_u(const Vec3 p[], Vec3 p1[], Vec3 p2[]) {
		p2[0] = (p[0] + p[1]) * 0.5;
		p2[1] = p[1];
		p2[2] = p[2];
		p2[3] = (p[2] + p[3]) * 0.5;

		p1[0] = p[0];
		p1[1] = (p[0] + p[1]) * 0.5;
		p1[2] = (p[2] + p[3]) * 0.5;
		p1[3] = p[3];
	}

	__attribute__((always_inline))
	static void split_v(const Vec3 p[], Vec3 p1[], Vec3 p2[]) {
		p2[0] = (p[3] + p[0]) * 0.5;
		p2[1] = (p[1] + p[2]) * 0.5;
		p2[2] = p[2];
		p2[3] = p[3];

		p1[0] = p[0];
		p1[1] = p[1];
		p1[2] = (p[1] + p[2]) * 0.5;
		p1[3] = (p[3] + p[0]) * 0.5;
	}

	static Vec3 dp_u(const Vec3 p[], float u, float v) {
		// First we interpolate across v to get a curve
		const float iv = 1.0f - v;
		Vec3 c[2];
		c[0] = (p[0] * iv) + (p[3] * v);
		c[1] = (p[1] * iv) + (p[2] * v);

		// Now we use the derivatives across u to find dp
		return c[1] - c[0];
	}

	static Vec3 dp_v(const Vec3 p[], float u, float v) {

		// First we interpolate across u to get a curve
		const float iu = 1.0f - u; // We use this a lot, so pre-calculate
		Vec3 c[2];
		c[0] = (p[0] * iu) + (p[1] * u);
		c[1] = (p[3] * iu) + (p[2] * u);;

		// Now we use the derivatives across u to find dp
		return c[1] - c[0];
	}

	__attribute__((always_inline))
	static BBox bound(const std::array<Vec3, 4>& p) {
		BBox bb = BBox(p[0], p[0]);;

		for (int i = 1; i < 4; ++i) {
			bb.min = min(bb.min, p[i]);
			bb.max = max(bb.max, p[i]);
		}

		return bb;
	}
};

#endif
