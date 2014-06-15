#ifndef LIGHT_TREE_HPP
#define LIGHT_TREE_HPP

#include "light_accel.hpp"
#include <iostream>

class LightTree: public LightAccel
{
	struct BuildNode {
		Light* light;
		Vec3 center;
		BBox bbox;
		float energy;
	};

	struct Node {
		BBox bbox;
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

		if (mid_itr == start)
			mid_itr++;

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
			nodes[me].bbox = start->bbox;

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
			nodes[me].bbox = nodes[c1].bbox | nodes[c2].bbox;

			// Calculate energy
			nodes[me].energy = nodes[c1].energy + nodes[c2].energy;
		}

		return me;
	}

	float node_prob(Vec3 p, Vec3 nor, uint32_t index) const {
		const Vec3 d = nodes[index].bbox.center() - p;
		const float dist2 = d.length2();
		const float r = nodes[index].bbox.diagonal() * 0.5f;
		const float r2 = r * r;
		const float inv_surface_area = 1.0f / r2;

		const float sin_theta_max2 = std::min(1.0f, r2 / dist2);
		const float cos_theta_max = std::sqrt(1.0f - sin_theta_max2);

		// TODO: why does this work so well?  Specifically: does
		// it also work well with BSDF's other than lambert?
		float frac = (dot(nor, d) + r) / std::sqrt(dist2);
		frac = std::max(0.0f, std::min(1.0f, frac));

		// An alternative to the above that's supposedly more "generic",
		// because it's just expressing the fraction of the light that's
		// above the surface's horizon.
		// // float frac = (dot(nor, d) + r) / (r * 2.0f);
		// // frac = std::max(0.0f, std::min(1.0f, frac));

		return nodes[index].energy * inv_surface_area * (1.0 - cos_theta_max) * frac;
	}


public:
	~LightTree() {}

	virtual void build(const Assembly& assembly);

	virtual void sample(LightQuery* query) {
//		Node* node = &(nodes[0]);
//
//		float tot_prob = 1.0f;
//
//		// Traverse down the tree, keeping track of the relative probabilities
//		while (true) {
//			if (node->is_leaf)
//				break;
//
//			// Calculate the relative probabilities of the two children
//			float p1 = node_prob(pos, nor, node->index1);
//			float p2 = node_prob(pos, nor, node->index2);
//			const float total = p1 + p2;
//			if (total <= 0.0f) {
//				p1 = 0.5f;
//				p2 = 0.5f;
//			} else {
//				p1 /= total;
//				p2 /= total;
//			}
//
//			if (n <= p1) {
//				tot_prob *= p1;
//				node = &(nodes[node->index1]);
//				n /= p1;
//			} else {
//				tot_prob *= p2;
//				node = &(nodes[node->index2]);
//				n = (n - p1) / p2;
//			}
//		}
//
//		// Return the selected light and it's probability
//		return std::make_tuple(node->light, tot_prob);
	}

	// TODO
	virtual size_t light_count() const {
		return 1;
	}
};

#endif // LIGHT_TREE_HPP
