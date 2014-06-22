#ifndef LIGHT_TREE_HPP
#define LIGHT_TREE_HPP

#include "light_accel.hpp"
#include <iostream>

class LightTree: public LightAccel
{
	struct BuildNode {
		size_t instance_index;
		Vec3 center;
		BBox bbox;
		float energy;
	};

	struct Node {
		std::vector<BBox> bounds;
		float energy;

		size_t index1;
		size_t index2;

		bool is_leaf;
		size_t instance_index;
	};

	const Assembly* assembly;
	std::vector<BuildNode> build_nodes;
	std::vector<Node> nodes;
	std::vector<BBox> bounds_;
	float total_energy {0.0f};
	size_t total_lights {0};

	std::vector<BuildNode>::iterator split_lights(std::vector<BuildNode>::iterator start, std::vector<BuildNode>::iterator end);
	size_t recursive_build(std::vector<BuildNode>::iterator start, std::vector<BuildNode>::iterator end);

	float node_prob(const LightQuery& lq, uint32_t index) const;


public:
	~LightTree() {}

	virtual void build(const Assembly& assembly) override;

	virtual void sample(LightQuery* query) const override;


	virtual const std::vector<BBox>& bounds() const override {
		return bounds_;
	}


	// TODO
	virtual size_t light_count() const override {
		return total_lights;
	}


	virtual Color total_emitted_color() const override {
		return Color(total_energy);
	}
};

#endif // LIGHT_TREE_HPP
