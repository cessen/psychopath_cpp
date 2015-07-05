#ifndef LIGHT_ACCEL_HPP
#define LIGHT_ACCEL_HPP

#include <vector>
#include <tuple>
#include <memory>

#include "numtype.h"
#include "instance_id.hpp"
#include "ray.hpp"
#include "light.hpp"
#include "transform.hpp"
#include "color.hpp"


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
	Vec3 d;  // Direction of the known ray
	SurfaceClosure* bsdf;
	float wavelength;
	float time;

	// Intermediate
	Transform xform;

	// Out
	InstanceID id;
	Vec3 to_light;
	SpectralSample spec_samp;
	float selection_pdf;  // The pdf of selecting the given light
	float light_sample_pdf;  // The pdf of the sample taken on the selected light
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
