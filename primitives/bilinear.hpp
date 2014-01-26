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
public:
	std::vector<std::array<Vec3, 4>> verts;
	float u_min {0.0f}, v_min {0.0f};
	float u_max {1.0f}, v_max {1.0f};
	float longest_u, longest_v;
	float log_widest = 0.0f;  // Log base 2 of the widest part of the patch, for fast subdivision rate estimates

	BBoxT bbox;

	Bilinear() {}
	Bilinear(Vec3 v1, Vec3 v2, Vec3 v3, Vec3 v4);
	virtual ~Bilinear() {}

	void finalize();

	void add_time_sample(Vec3 v1, Vec3 v2, Vec3 v3, Vec3 v4);
	Grid *grid_dice(const int ru, const int rv);

	virtual BBoxT &bounds();

	virtual int split(std::unique_ptr<DiceableSurfacePrimitive> primitives[]);
	virtual std::unique_ptr<DiceableSurfacePrimitive> copy();
	virtual size_t subdiv_estimate(float width) const;
	virtual std::shared_ptr<MicroSurface> dice(size_t subdivisions);
};

#endif
