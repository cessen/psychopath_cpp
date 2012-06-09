#ifndef LIGHT_HPP
#define LIGHT_HPP

#include "vector.hpp"
#include "color.hpp"

/**
 * @brief An interface for finite light sources.
 */
class FiniteLight
{
public:
	virtual ~FiniteLight() {}

	/**
	 * @brief Returns a 3d point 3d that lies on the light source.
	 *
	 * The point returned is generated based on the parameters u, v, and time.
	 * The same input parameters will always result in the same point.
	 *
	 * @param u Surface parameter U.
	 * @param u Surface parameter V.
	 * @param time The time to sample at.
	 */
	virtual Vec3 get_sample_position(float u, float v, float time) const = 0;


	/**
	 * @brief Returns the color emitted in the given direction from the
	 * given parameters on the light.
	 *
	 * @param dir The direction of the outgoing light.
	 * @param u Surface parameter U.
	 * @param u Surface parameter V.
	 * @param time The time to sample at.
	 */
	virtual Color outgoing_light(Vec3 dir, float u, float v, float time) const = 0;
};


/**
 * @brief An interface for infinite light sources.
 */
class InfiniteLight
{
public:
	virtual ~InfiniteLight() {}

	/**
	 * @brief Returns a 3d direction coming from the light source.
	 *
	 * The direction returned is generated based on the parameters u, v, and
	 * time.  The same input parameters will always result in the same
	 * direction.
	 *
	 * @param u Surface parameter U.
	 * @param u Surface parameter V.
	 * @param time The time to sample at.
	 */
	virtual Vec3 get_sample_direction(float u, float v, float time) const = 0;


	/**
	 * @brief Returns the color emitted in the given direction from the light.
	 *
	 * @param u Surface parameter U.
	 * @param u Surface parameter V.
	 * @param time The time to sample at.
	 */
	virtual Color outgoing_light(float u, float v, float time) const = 0;
};

#endif // LIGHT_HPP
