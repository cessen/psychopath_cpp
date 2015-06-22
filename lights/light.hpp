#ifndef LIGHT_HPP
#define LIGHT_HPP

#include "object.hpp"
#include "bbox.hpp"
#include "vector.hpp"
#include "color.hpp"

/**
 * @brief An interface for light sources.
 */
class Light: public Object
{
public:
	virtual ~Light() {}

	Object::Type get_type() const final {
		return Object::LIGHT;
	}

	/**
	 * @brief Samples the light source for a given point to be illuminated.
	 *
	 * @param arr The point to be illuminated.
	 * @param u Random parameter U.
	 * @param v Random parameter V.
	 * @param wavelength The wavelength of light to sample at.
	 * @param time The time to sample at.
	 * @param[out] shadow_vec The world-space direction to cast a shadow ray
	 *               for visibility testing.  It's length determines the extent
	 *               that the shadow ray should have, unless the light source
	 *               is infinite (see is_infinite()) in which case the extent
	 *               should be infinite.  This vector also doubles to inform
	 *               What direction the light is arriving from (just invert
	 *               the vector).
	 *
	 * @returns The light arriving at the point arr.
	 */
	virtual SpectralSample sample(const Vec3 &arr, float u, float v, float wavelength, float time,
	                              Vec3 *shadow_vec, float* pdf) const = 0;


	/**
	 * @brief Returns the color emitted in the given direction from the
	 * given parameters on the light.
	 *
	 * @param dir The direction of the outgoing light.
	 * @param u Random parameter U.
	 * @param v Random parameter V.
	 * @param wavelength The wavelength of light to sample at.
	 * @param time The time to sample at.
	 */
	virtual SpectralSample outgoing(const Vec3 &dir, float u, float v, float wavelength, float time) const = 0;


	/**
	 * @brief Returns whether the light has a delta distribution.
	 *
	 * If a light has no chance of a ray hitting it through random process
	 * then it is a delta light source.  For example, point light sources,
	 * lights that only emit in a single direction, etc.
	 */
	virtual bool is_delta() const = 0;


	/**
	 * @brief Tests a ray against the light.
	 */
	virtual bool intersect_ray(const Ray &ray, Intersection *intersection=nullptr) const = 0;
};

#endif // LIGHT_HPP
