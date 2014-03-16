#ifndef LIGHT_ACCEL_HPP
#define LIGHT_ACCEL_HPP

#include "numtype.h"
#include "ray.hpp"
#include "scene_graph.hpp"
#include "light.hpp"

#include <vector>
#include <tuple>
#include <memory>


/**
 * @brief An acceleration structure for sampling a collection of light sources.
 */
class LightAccel
{
public:
	virtual ~LightAccel() {}

	virtual void build(const SceneGraph& scene_graph) = 0;

	virtual std::tuple<Light*, float> sample(Vec3 pos, float n) = 0;
};


#endif // LIGHT_ACCEL_HPP
