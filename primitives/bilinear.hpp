#ifndef BILINEAR_HPP
#define BILINEAR_HPP

#include "numtype.h"

#include <atomic>
#include <vector>
#include "vector.hpp"
#include "grid.hpp"
#include "primitive.hpp"
#include "timebox.hpp"

/*
 * A bilinear patch.
 * Vertices arranged like this:
 *     u-->
 *   v1----v2
 * v  |    |
 * | v4----v3
 * \/
 */
class Bilinear: public DiceableSurfacePrimitive
{
	void uv_dice_rate(size_t *u_rate, size_t *v_rate, float width) const {
		// longest u-side  and v-side of the patch
		const float ul = (verts[0][0] - verts[0][1]).length() > (verts[0][2] - verts[0][3]).length() ? (verts[0][0] - verts[0][1]).length() : (verts[0][2] - verts[0][3]).length();
		const float vl = (verts[0][0] - verts[0][3]).length() > (verts[0][1] - verts[0][2]).length() ? (verts[0][0] - verts[0][3]).length() : (verts[0][1] - verts[0][2]).length();

		// Dicing rates in u and v based on target microelement width
		*u_rate = ul / (width * Config::dice_rate);
		if (*u_rate < 1)
			*u_rate = 1;
		*v_rate = vl / (width * Config::dice_rate);
		if (*v_rate < 1)
			*v_rate = 1;

		//if(*u_rate == 1 && *v_rate == 1)
		//	std::cout << width << " " << *u_rate << " " << *v_rate << std::endl;
	}

public:
	TimeBox<Vec3 *> verts;
	float u_min, v_min, u_max, v_max;


	BBoxT bbox;
	bool has_bounds;

	Bilinear(uint16_t res_time_);
	Bilinear(Vec3 v1, Vec3 v2, Vec3 v3, Vec3 v4);
	virtual ~Bilinear();

	void add_time_sample(int samp, Vec3 v1, Vec3 v2, Vec3 v3, Vec3 v4);
	Grid *grid_dice(const int ru, const int rv);

	virtual BBoxT &bounds();

	virtual void split(std::vector<DiceableSurfacePrimitive *> &primitives);
	virtual size_t subdiv_estimate(float width) const;
	virtual std::shared_ptr<MicroSurface> dice(size_t subdivisions);
};

#endif
