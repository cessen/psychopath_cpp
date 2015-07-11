#ifndef BICUBIC_HPP
#define BICUBIC_HPP

#include "numtype.h"

#include <vector>
#include <array>
#include <tuple>
#include "utils.hpp"
#include "stack.hpp"
#include "vector.hpp"
#include "object.hpp"

/*
 * A bicubic bezier patch.
 * Vertices arranged like this:
 *     u-->
 *   v0----v1----v2----v3
 * v  |     |     |     |
 * | v4----v5----v6----v7
 * \/ |     |     |     |
 *   v8----v9----v10---v11
 *    |     |     |     |
 *   v12---v13---v14---v15
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

	static store_type interpolate_patch(float alpha, const store_type& p1, const store_type& p2) {
		store_type p3;
		for (int i = 0; i < 16; ++i) {
			p3[i] = lerp(alpha, p1[i], p2[i]);
		}
		return p3;
	}

	__attribute__((always_inline))
	static float ulen(const store_type &p) {
		return longest_axis(p[0] - p[3]);
	}

	__attribute__((always_inline))
	static float vlen(const store_type &p) {
		return longest_axis(p[0] - p[4*3]);
	}

	__attribute__((always_inline))
	static void split_u(const store_type& p, store_type* p1, store_type *p2) {
		for (int r = 0; r < 4; ++r) {
			const auto rr = (r * 4);
			Vec3 tmp = (p[rr+1] + p[rr+2]) * 0.5;

			(*p2)[rr+3] = p[rr+3];
			(*p2)[rr+2] = (p[rr+3] + p[rr+2]) * 0.5;
			(*p2)[rr+1] = (tmp + (*p2)[rr+2]) * 0.5;

			(*p1)[rr+0] = p[rr+0];
			(*p1)[rr+1] = (p[rr+0] + p[rr+1]) * 0.5;
			(*p1)[rr+2] = (tmp + (*p1)[rr+1]) * 0.5;

			(*p1)[rr+3] = ((*p1)[rr+2] + (*p2)[rr+1]) * 0.5;
			(*p2)[rr+0] = (*p1)[rr+3];
		}
	}

	__attribute__((always_inline))
	static void split_v(const store_type& p, store_type* p1, store_type* p2) {
		for (int c = 0; c < 4; ++c) {
			Vec3 tmp = (p[c+(1*4)] + p[c+(2*4)]) * 0.5;

			(*p2)[c+(3*4)] = p[c+(3*4)];
			(*p2)[c+(2*4)] = (p[c+(3*4)] + p[c+(2*4)]) * 0.5;
			(*p2)[c+(1*4)] = (tmp + (*p2)[c+(2*4)]) * 0.5;

			(*p1)[c+(0*4)] = p[c+(0*4)];
			(*p1)[c+(1*4)] = (p[c+(0*4)] + p[c+(1*4)]) * 0.5;
			(*p1)[c+(2*4)] = (tmp + (*p1)[c+(1*4)]) * 0.5;

			(*p1)[c+(3*4)] = ((*p1)[c+(2*4)] + (*p2)[c+(1*4)]) * 0.5;
			(*p2)[c+(0*4)] = (*p1)[c+(3*4)];
		}
	}

	static Vec3 eval_p(float u, const Vec3 p0, const Vec3 p1, const Vec3 p2, const Vec3 p3) {
		const float iu = 1.0f - u;
		const float b0 = iu * iu * iu;
		const float b1 = 3.0f * u * iu * iu;
		const float b2 = 3.0f * u * u * iu;
		const float b3 = u * u * u;

		return (p0 * b0) + (p1 * b1) + (p2 * b2) + (p3 * b3);
	}

	static Vec3 eval_pd(float u, const Vec3 p0, const Vec3 p1, const Vec3 p2, const Vec3 p3) {
		const float iu = 1.0f - u;
		const float d0 = -3.0f * iu * iu;
		const float d1 = (3.0f * iu * iu) - (6.0f * iu * u);
		const float d2 = (6.0f * iu * u) - (3.0f * u * u);
		const float d3 = 3.0f * u * u;

		return (p0 * d0) + (p1 * d1) + (p2 * d2) + (p3 * d3);
	}

	static Vec3 eval_pdd(float u, const Vec3 p0, const Vec3 p1, const Vec3 p2, const Vec3 p3) {
		const float iu = 1.0f - u;
		const float dd0 = 6.0f * iu;
		const float dd1 = (6.0f * u) - (12.0f * iu);
		const float dd2 = (6.0f * iu) - (12.0f * u);
		const float dd3 = 6.0f * u;

		return (p0 * dd0) + (p1 * dd1) + (p2 * dd2) + (p3 * dd3);
	}

	/**
	 * Returns <n, dpdu, dpdv, dndu, dndv>
	 */
	static std::tuple<Vec3, Vec3, Vec3, Vec3, Vec3> differential_geometry(const store_type& p, float u, float v) {
		// Eval points along u and v, and derivatives of u along v
		Vec3 pu[4];  // Points along u direction at v
		Vec3 pv[4];  // Points along v direction at u
		Vec3 pdv[4]; // Derivatives of u along v direction
		for (int i = 0; i < 4; ++i) {
			pu[i] = eval_p(v, p[i], p[i+4], p[i+8], p[i+12]);
			pv[i] = eval_p(u, p[i*4], p[i*4+1], p[i*4+2], p[i*4+3]);
			pdv[i] = eval_pd(u, p[i*4], p[i*4+1], p[i*4+2], p[i*4+3]);
		}

		// Calculate first derivatives and surface normal
		const Vec3 dpdu = eval_pd(u, pu[0], pu[1], pu[2], pu[3]);
		const Vec3 dpdv = eval_pd(v, pv[0], pv[1], pv[2], pv[3]);
		const Vec3 n = cross(dpdv, dpdu).normalized();

		// Calculate second derivatives
		const Vec3 d2pduu = eval_pdd(u, pu[0], pu[1], pu[2], pu[3]);
		const Vec3 d2pduv = eval_pd(v, pdv[0], pdv[1], pdv[2], pdv[3]);
		const Vec3 d2pdvv = eval_pdd(v, pv[0], pv[1], pv[2], pv[3]);

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
		BBox bb = BBox(p[0], p[0]);

		for (int i = 1; i < 16; ++i) {
			bb.min = min(bb.min, p[i]);
			bb.max = max(bb.max, p[i]);
		}

		return bb;
	}
};

#endif // BICUBIC
