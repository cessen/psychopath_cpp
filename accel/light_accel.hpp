#ifndef LIGHT_ACCEL_HPP
#define LIGHT_ACCEL_HPP

#include "numtype.h"
#include "ray.hpp"
#include "light.hpp"

#include <vector>
#include <tuple>
#include <memory>

// Forward declaration of Assembly from scene/assembly.hpp
class Assembly;

/**
 * @brief An acceleration structure for sampling a collection of light sources.
 */
class LightAccel
{
public:
	virtual ~LightAccel() {}

	virtual void build(const Assembly& assembly) = 0;

	virtual std::tuple<Light*, float> sample(Vec3 pos, Vec3 nor, float n) = 0;
};


#endif // LIGHT_ACCEL_HPP
