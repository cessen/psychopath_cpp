#ifndef BILINEAR_HPP
#define BILINEAR_HPP

#include "numtype.h"

#include <vector>
#include <array>
#include "utils.hpp"
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
 * | v3----v4
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

	static store_type interpolate_patch(float alpha, const store_type& p1, const store_type& p2) {
		store_type p3;
		for (int i = 0; i < 4; ++i) {
			p3[i] = lerp(alpha, p1[i], p2[i]);
		}
		return p3;
	}

	__attribute__((always_inline))
	static float ulen(const store_type& p) {
		return longest_axis(p[0] - p[1]);
	}

	__attribute__((always_inline))
	static float vlen(const store_type& p) {
		return longest_axis(p[0] - p[2]);
	}

	__attribute__((always_inline))
	static void split_u(const store_type& p, store_type* p1, store_type* p2) {
		(*p2)[0] = (p[0] + p[1]) * 0.5f;
		(*p2)[1] = p[1];
		(*p2)[2] = (p[2] + p[3]) * 0.5f;
		(*p2)[3] = p[3];

		(*p1)[0] = p[0];
		(*p1)[1] = (p[0] + p[1]) * 0.5f;
		(*p1)[2] = p[2];
		(*p1)[3] = (p[2] + p[3]) * 0.5f;
	}

	__attribute__((always_inline))
	static void split_v(const store_type& p, store_type* p1, store_type* p2) {
		(*p2)[0] = (p[0] + p[2]) * 0.5f;
		(*p2)[1] = (p[1] + p[3]) * 0.5f;
		(*p2)[2] = p[2];
		(*p2)[3] = p[3];

		(*p1)[0] = p[0];
		(*p1)[1] = p[1];
		(*p1)[2] = (p[0] + p[2]) * 0.5f;
		(*p1)[3] = (p[1] + p[3]) * 0.5f;
	}

	/**
	 * Returns <n, dpdu, dpdv, dndu, dndv>
	 */
	static std::tuple<Vec3, Vec3, Vec3, Vec3, Vec3> differential_geometry(const store_type& p, float u, float v) {
		// Calculate first derivatives and surface normal
		const Vec3 dpdu = ((p[0]-p[1]) * v) - (p[2] * v) + p[2] + (p[3] * (v-1.0f));
		const Vec3 dpdv = ((p[0]-p[2]) * u) - (p[1] * u) + p[1] + (p[3] * (u-1.0f));
		const Vec3 n = cross(dpdv, dpdu).normalized();

		// Calculate second derivatives
		const Vec3 d2pduu = Vec3(0.0f);
		const Vec3 d2pduv = p[0] - p[1] - p[2] + p[3];
		const Vec3 d2pdvv = Vec3(0.0f);

		// Calculate surface normal derivatives
		const float E = dot(dpdu, dpdu);
		const float F = dot(dpdu, dpdv);
		const float G = dot(dpdv, dpdv);
		const float e = dot(n, d2pduu);
		const float f = dot(n, d2pduv);
		const float g = dot(n, d2pdvv);

		const float invEGF2 = 1.0f / ((E*G) - (F*F));
		const Vec3 dndu = (((f*F) - (e*G)) * invEGF2 * dpdu) + (((e*F) - (f*E)) * invEGF2 * dpdv);
		const Vec3 dndv = (((g*F) - (f*G)) * invEGF2 * dpdu) + (((f*F) - (g*E)) * invEGF2 * dpdv);

		return std::make_tuple(n, dpdu, dpdv, dndu, dndv);
	}

	__attribute__((always_inline))
	static BBox bound(const store_type& p) {
		BBox bb = BBox(p[0], p[0]);;

		for (int i = 1; i < 4; ++i) {
			bb.min = min(bb.min, p[i]);
			bb.max = max(bb.max, p[i]);
		}

		return bb;
	}
};

#endif
