#ifndef BICUBIC_HPP
#define BICUBIC_HPP

#include "numtype.h"

#include <atomic>
#include <vector>
#include <array>
#include "vector.hpp"
#include "grid.hpp"
#include "primitive.hpp"

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
class Bicubic: public DiceableSurfacePrimitive
{
public:
	std::vector<std::array<Vec3, 16>> verts;
	float u_min, v_min, u_max, v_max;
	float longest_u = 0.0f;
	float longest_v = 0.0f;

	BBoxT bbox;

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
	Grid *grid_dice(const int ru, const int rv);

	virtual BBoxT &bounds();

	virtual int split(std::unique_ptr<DiceableSurfacePrimitive> primitives[]);
	virtual std::unique_ptr<DiceableSurfacePrimitive> copy();
	virtual size_t subdiv_estimate(float width) const;
	virtual std::shared_ptr<MicroSurface> dice(size_t subdivisions);
};

#endif // BICUBIC
