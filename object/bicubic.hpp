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
class Bicubic final: public BreadthSurface
{
public:
	std::vector<std::array<Vec3, 16>> verts;
	float u_min {0.0f}, v_min {0.0f};
	float u_max {1.0f}, v_max {1.0f};
	float longest_u, longest_v;
	float log_widest = 0.0f;  // Log base 2 of the widest part of the patch, for fast subdivision rate estimates

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

	/*
	Grid *grid_dice(const int ru, const int rv) const;
	virtual int split(std::unique_ptr<DiceableSurface> objects[]) override;
	virtual std::unique_ptr<DiceableSurface> copy() const override;
	virtual size_t subdiv_estimate(float width) const override;
	virtual std::shared_ptr<MicroSurface> dice(size_t subdivisions) const override;*/

	virtual bool intersect_single_ray_helper(const Ray &ray, const std::array<Vec3, 16> &patch, const std::array<Vec3, 16> &subpatch, const std::tuple<float, float, float, float> &uvs, Intersection intersections[]);
	virtual void intersect_rays(const std::vector<Transform>& parent_xforms, Ray* ray_begin, Ray* ray_end, Intersection *intersections, Stack* data_stack);
};

#endif // BICUBIC
