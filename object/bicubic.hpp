#ifndef BICUBIC_HPP
#define BICUBIC_HPP

#include "numtype.h"

#include <atomic>
#include <vector>
#include <array>
#include "stack.hpp"
#include "vector.hpp"
#include "grid.hpp"
#include "object.hpp"

/*
 * A bicubic bezier patch.
 * Vertices arranged like this:
 *     u-->
 *   v1----v2----v3----v4
 * v  |     |     |     |
 * | v5----v6----v7----v8
 * \/ |     |     |     |
 *   v9----v10---v11---v12
 *    |     |     |     |
 *   v13---v14---v15---v16
 */
class Bicubic final: public PatchSurface
{
public:
	std::vector<std::array<Vec3, 16>> verts;
	std::vector<BBox> bbox;

	Bicubic() {};
	Bicubic(Vec3 v1,  Vec3 v2,  Vec3 v3,  Vec3 v4,
	        Vec3 v5,  Vec3 v6,  Vec3 v7,  Vec3 v8,
	        Vec3 v9,  Vec3 v10, Vec3 v11, Vec3 v12,
	        Vec3 v13, Vec3 v14, Vec3 v15, Vec3 v16);
	virtual ~Bicubic() {};

	void add_time_sample(Vec3 v1,  Vec3 v2,  Vec3 v3,  Vec3 v4,
	                     Vec3 v5,  Vec3 v6,  Vec3 v7,  Vec3 v8,
	                     Vec3 v9,  Vec3 v10, Vec3 v11, Vec3 v12,
	                     Vec3 v13, Vec3 v14, Vec3 v15, Vec3 v16);
	void finalize();

	virtual const std::vector<BBox> &bounds() const override;
	virtual Color total_emitted_color() const override {
		return Color(0.0f);
	}


	// For being traced by intersect_rays_with_patch() in tracer.cpp
	typedef std::array<Vec3, 16> store_type;

	__attribute__((always_inline))
	static float ulen(const std::array<Vec3, 16> &p) {
		return longest_axis(p[0] - p[3]);
	}

	__attribute__((always_inline))
	static float vlen(const std::array<Vec3, 16> &p) {
		return longest_axis(p[0] - p[4*3]);
	}

	__attribute__((always_inline))
	static void split_u(const Vec3 p[], Vec3 p1[], Vec3 p2[]) {
		for (int r = 0; r < 4; ++r) {
			const auto rr = (r * 4);
			Vec3 tmp = (p[rr+1] + p[rr+2]) * 0.5;

			p2[rr+3] = p[rr+3];
			p2[rr+2] = (p[rr+3] + p[rr+2]) * 0.5;
			p2[rr+1] = (tmp + p2[rr+2]) * 0.5;

			p1[rr+0] = p[rr+0];
			p1[rr+1] = (p[rr+0] + p[rr+1]) * 0.5;
			p1[rr+2] = (tmp + p1[rr+1]) * 0.5;

			p1[rr+3] = (p1[rr+2] + p2[rr+1]) * 0.5;
			p2[rr+0] = p1[rr+3];
		}
	}

	__attribute__((always_inline))
	static void split_v(const Vec3 p[], Vec3 p1[], Vec3 p2[]) {
		for (int c = 0; c < 4; ++c) {
			Vec3 tmp = (p[c+(1*4)] + p[c+(2*4)]) * 0.5;

			p2[c+(3*4)] = p[c+(3*4)];
			p2[c+(2*4)] = (p[c+(3*4)] + p[c+(2*4)]) * 0.5;
			p2[c+(1*4)] = (tmp + p2[c+(2*4)]) * 0.5;

			p1[c+(0*4)] = p[c+(0*4)];
			p1[c+(1*4)] = (p[c+(0*4)] + p[c+(1*4)]) * 0.5;
			p1[c+(2*4)] = (tmp + p1[c+(1*4)]) * 0.5;

			p1[c+(3*4)] = (p1[c+(2*4)] + p2[c+(1*4)]) * 0.5;
			p2[c+(0*4)] = p1[c+(3*4)];
		}
	}

	static Vec3 dp_u(const Vec3 p[], float u, float v) {

		// First we interpolate across v to get a curve
		const float iv = 1.0f - v;
		const float b0 = iv * iv * iv;
		const float b1 = 3.0f * v * iv * iv;
		const float b2 = 3.0f * v * v * iv;
		const float b3 = v * v * v;
		Vec3 c[4];
		c[0] = (p[0] * b0) + (p[4] * b1) + (p[8] * b2) + (p[12] * b3);
		c[1] = (p[1] * b0) + (p[5] * b1) + (p[9] * b2) + (p[13] * b3);
		c[2] = (p[2] * b0) + (p[6] * b1) + (p[10] * b2) + (p[14] * b3);
		c[3] = (p[3] * b0) + (p[7] * b1) + (p[11] * b2) + (p[15] * b3);

		// Now we use the derivatives across u to find dp
		const float iu = 1.0f - u;
		const float d0 = -3.0f * iu * iu;
		const float d1 = (3.0f * iu * iu) - (6.0f * iu * u);
		const float d2 = (6.0f * iu * u) - (3.0f * u * u);
		const float d3 = 3.0f * u * u;

		return (c[0] * d0) + (c[1] * d1) + (c[2] * d2) + (c[3] * d3);
	}

	static Vec3 dp_v(const Vec3 p[], float u, float v) {

		// First we interpolate across u to get a curve
		const float iu = 1.0f - u; // We use this a lot, so pre-calculate
		const float b0 = iu * iu * iu;
		const float b1 = 3.0f * u * iu * iu;
		const float b2 = 3.0f * u * u * iu;
		const float b3 = u * u * u;
		Vec3 c[4];
		c[0] = (p[0] * b0) + (p[1] * b1) + (p[2] * b2) + (p[3] * b3);
		c[1] = (p[4] * b0) + (p[5] * b1) + (p[6] * b2) + (p[7] * b3);
		c[2] = (p[8] * b0) + (p[9] * b1) + (p[10] * b2) + (p[11] * b3);
		c[3] = (p[12] * b0) + (p[13] * b1) + (p[14] * b2) + (p[15] * b3);

		// Now we use the derivatives across u to find dp
		const float iv = 1.0f - v; // We use this a lot, so pre-calculate
		const float d0 = -3.0f * iv * iv;
		const float d1 = (3.0f * iv * iv) - (6.0f * iv * v);
		const float d2 = (6.0f * iv * v) - (3.0f * v * v);
		const float d3 = 3.0f * v * v;

		return (c[0] * d0) + (c[1] * d1) + (c[2] * d2) + (c[3] * d3);
	}

	__attribute__((always_inline))
	static BBox bound(const std::array<Vec3, 16>& p) {
		BBox bb = BBox(p[0], p[0]);

		for (int i = 1; i < 16; ++i) {
			bb.min = min(bb.min, p[i]);
			bb.max = max(bb.max, p[i]);
		}

		return bb;
	}
};

#endif // BICUBIC
