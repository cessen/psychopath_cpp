#ifndef BILINEAR_HPP
#define BILINEAR_HPP

#include "numtype.h"

#include <atomic>
#include <vector>
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
class Bilinear final: public BreadthSurface
{
public:
	std::vector<std::array<Vec3, 4>> verts;
	float u_min {0.0f}, v_min {0.0f};
	float u_max {1.0f}, v_max {1.0f};
	float longest_u, longest_v;
	float log_widest = 0.0f;  // Log base 2 of the widest part of the patch, for fast subdivision rate estimates

	std::vector<BBox> bbox;

	Bilinear() {}
	Bilinear(Vec3 v1, Vec3 v2, Vec3 v3, Vec3 v4);
	virtual ~Bilinear() {}

	void finalize();

	void add_time_sample(Vec3 v1, Vec3 v2, Vec3 v3, Vec3 v4);
	//Grid *grid_dice(const int ru, const int rv) const;

	virtual const std::vector<BBox> &bounds() const override;
	virtual Color total_emitted_color() const override {
		return Color(0.0f);
	}

	//virtual int split(std::unique_ptr<DiceableSurface> objects[]) override;
	//virtual std::unique_ptr<DiceableSurface> copy() const override;
	//virtual size_t subdiv_estimate(float width) const override;
	//virtual std::shared_ptr<MicroSurface> dice(size_t subdivisions) const override;

	virtual void intersect_rays(const std::vector<Transform>& parent_xforms, Ray* ray_begin, Ray* ray_end, Intersection *intersections);
};

#endif
