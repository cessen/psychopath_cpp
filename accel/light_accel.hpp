#ifndef LIGHT_ACCEL_HPP
#define LIGHT_ACCEL_HPP

#include <vector>
#include <tuple>
#include <memory>

#include "numtype.h"
#include "ray.hpp"
#include "light.hpp"
#include "transform.hpp"


// Forward declaration of Assembly from scene/assembly.hpp
class Assembly;


/**
 * Data structure used to query for a light sample.
 */
struct LightQuery {
	// In
	float n, u, v, w;
	Vec3 pos;
	Vec3 nor;
	float time;

	// Intermediate
	Transform xform;

	// Out
	Vec3 to_light;
	Color color;
	float pdf;
};

/**
 * @brief An acceleration structure for sampling a collection of light sources.
 */
class LightAccel
{
public:
	virtual ~LightAccel() {}

	virtual void build(const Assembly& assembly) = 0;

	virtual void sample(LightQuery* query) const = 0;

	virtual const std::vector<BBox>& bounds() const = 0;

	virtual size_t light_count() const = 0;

	virtual Color total_emitted_color() const = 0;
};


#endif // LIGHT_ACCEL_HPP
