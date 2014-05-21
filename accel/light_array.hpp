#ifndef LIGHT_ARRAY_HPP
#define LIGHT_ARRAY_HPP

#include "light_accel.hpp"

class LightArray: public LightAccel
{
	std::vector<Light*> lights;

public:
	~LightArray() {}

	virtual void build(const Assembly& assembly);

	virtual std::tuple<Light*, float> sample(Vec3 pos, Vec3 nor, float n) {
		Light* light = lights[static_cast<uint32_t>(n * lights.size()) % lights.size()];
		float p = 1.0f / lights.size();

		return std::make_tuple(light, p);
	}
};

#endif // LIGHT_ARRAY_HPP