#ifndef LIGHT_ARRAY_HPP
#define LIGHT_ARRAY_HPP

#include "light_accel.hpp"

class LightArray final: public LightAccel {
	const Assembly* assembly;
	std::vector<size_t> light_indices;
	std::vector<std::tuple<size_t, size_t, size_t>> assembly_lights;  // 1: accumulated total lights, 2: number of light, 3: assembly instance index
	size_t total_assembly_lights;
	Color total_color;
	std::vector<BBox> bounds_ {BBox()};

public:
	~LightArray() {}

	virtual void build(const Assembly& assembly);

	virtual void sample(LightQuery* query) const;

	virtual const std::vector<BBox>& bounds() const {
		return bounds_;
	}

	virtual size_t light_count() const {
		return total_assembly_lights + light_indices.size();
	}

	virtual Color total_emitted_color() const {
		return total_color;
	}
};

#endif // LIGHT_ARRAY_HPP
