#ifndef LIGHT_TREE_HPP
#define LIGHT_TREE_HPP

#include "light_accel.hpp"
#include <iostream>

class LightTree: public LightAccel
{
	struct BuildNode {
		Light* light;
		Vec3 center;
		float radius;
		float energy;
	};

	struct Node {
		Vec3 center;
		float radius;
		float energy;

		size_t index1;
		size_t index2;

		bool is_leaf;
		Light* light;
	};

	std::vector<BuildNode> build_nodes;
	std::vector<Node> nodes;

	struct CompareToMid {
		int32_t dim;
		float mid;

		CompareToMid(int32_t d, float m) {
			dim = d;
			mid = m;
		}

		bool operator()(BuildNode &a) const {
			return a.center[dim] < mid;
		}
	};

	std::vector<BuildNode>::iterator split_lights(std::vector<BuildNode>::iterator start, std::vector<BuildNode>::iterator end) {
		// Find the minimum and maximum centroid values on each axis
		Vec3 min, max;
		min = start->center;
		max = start->center;
		for (auto itr = start + 1; itr < end; itr++) {
			for (int32_t d = 0; d < 3; d++) {
				min[d] = min[d] < itr->center[d] ? min[d] : itr->center[d];
				max[d] = max[d] > itr->center[d] ? max[d] : itr->center[d];
			}
		}

		// Find the axis with the maximum extent
		int32_t max_axis = 0;
		if ((max.y - min.y) > (max.x - min.x))
			max_axis = 1;
		if ((max.z - min.z) > (max.y - min.y))
			max_axis = 2;

		// Sort and split the list
		float pmid = .5f * (min[max_axis] + max[max_axis]);
		auto mid_itr = std::partition(start,
		                              end,
		                              CompareToMid(max_axis, pmid));

		return mid_itr;
	}

	size_t recursive_build(std::vector<BuildNode>::iterator start, std::vector<BuildNode>::iterator end) {
		// Allocate the node
		const size_t me = nodes.size();
		nodes.push_back(Node());

		if ((start + 1) == end) {
			// Leaf node

			nodes[me].is_leaf = true;
			nodes[me].light = start->light;

			// Copy bounds
			nodes[me].center = start->center;
			nodes[me].radius = start->radius;

			// Copy energy
			nodes[me].energy = start->energy;
		} else {
			// Not a leaf node
			nodes[me].is_leaf = false;

			// Create child nodes
			auto split_itr = split_lights(start, end);
			const size_t c1 = recursive_build(start, split_itr);
			const size_t c2 = recursive_build(split_itr, end);
			nodes[me].index1 = c1;
			nodes[me].index2 = c2;

			// Calculate bounds
			nodes[me].center = (nodes[c1].center + nodes[c2].center) * 0.5f;
			nodes[me].radius = std::abs((nodes[c1].center - nodes[c2].center).length()) * 0.5f;
			nodes[me].radius += std::max(nodes[c1].radius, nodes[c2].radius);

			// Calculate energy
			nodes[me].energy = nodes[c1].energy + nodes[c2].energy;
		}

		return me;
	}

	float node_prob(Vec3 p, uint32_t index) const {
		const float d = (p - nodes[index].center).length() - nodes[index].radius;

		if (d > 0) {
			return nodes[index].energy / ((d * d) + 1.0f);
		} else {
			return nodes[index].energy;
		}
	}


public:
	~LightTree() {}

	virtual void build(const SceneGraph& scene_graph) {
		// Populate the build nodes
		for (auto& l: scene_graph.finite_lights) {
			Light* light = l.second.get();
			BBox bbox = light->bounds();

			build_nodes.push_back(BuildNode());
			build_nodes.back().light = light;
			build_nodes.back().center = bbox.center();
			build_nodes.back().radius = bbox.diagonal() * 0.5f;
			build_nodes.back().energy = light->total_energy();
		}

		recursive_build(build_nodes.begin(), build_nodes.end());
	}

	virtual std::tuple<Light*, float> sample(Vec3 pos, float n) {
		Node* node = &(nodes[0]);

		float tot_prob = 1.0f;

		// Traverse down the tree, keeping track of the relative probabilities
		while (true) {
			if (node->is_leaf)
				break;

			// Calculate the relative probabilities of the two children
			float p1 = node_prob(pos, node->index1);
			float p2 = node_prob(pos, node->index2);
			const float total = p1 + p2;
			p1 /= total;
			p2 /= total;

			if (n <= p1) {
				tot_prob *= p1;
				node = &(nodes[node->index1]);
				n /= p1;
			} else {
				tot_prob *= p2;
				node = &(nodes[node->index2]);
				n = (n - p1) / p2;
			}
		}

		// Return the selected light and it's probability
		return std::make_tuple(node->light, tot_prob);
	}
};

#endif // LIGHT_TREE_HPP