#include "light_tree.hpp"

#include "assembly.hpp"


float LightTree::node_prob(const LightQuery& lq, uint32_t index) const
{
	const BBox bbox = lerp_seq(lq.time, nodes[index].bounds);
	const Vec3 d = bbox.center() - lq.pos;
	const float dist2 = d.length2();
	const float r = bbox.diagonal() * 0.5f;
	const float r2 = r * r;
	const float inv_surface_area = 1.0f / r2;

	const float sin_theta_max2 = std::min(1.0f, r2 / dist2);
	const float cos_theta_max = std::sqrt(1.0f - sin_theta_max2);

	// TODO: why does this work so well?  Specifically: does
	// it also work well with BSDF's other than lambert?
	float frac = (dot(lq.nor, d) + r) / std::sqrt(dist2);
	frac = std::max(0.0f, std::min(1.0f, frac));

	// An alternative to the above that's supposedly more "generic",
	// because it's just expressing the fraction of the light that's
	// above the surface's horizon.
	// // float frac = (dot(lq.nor, d) + r) / (r * 2.0f);
	// // frac = std::max(0.0f, std::min(1.0f, frac));

	return nodes[index].energy * inv_surface_area * (1.0 - cos_theta_max) * frac;
}


void LightTree::sample(LightQuery* query) const
{
	const Node* node = &(nodes[0]);

	float tot_prob = 1.0f;

	// Traverse down the tree, keeping track of the relative probabilities
	while (!node->is_leaf) {
		// Calculate the relative probabilities of the two children
		float p1 = node_prob(*query, node->index1);
		float p2 = node_prob(*query, node->index2);
		const float total = p1 + p2;
		if (total <= 0.0f) {
			p1 = 0.5f;
			p2 = 0.5f;
		} else {
			p1 /= total;
			p2 /= total;
		}

		if (query->n <= p1) {
			tot_prob *= p1;
			node = &(nodes[node->index1]);
			query->n /= p1;
		} else {
			tot_prob *= p2;
			node = &(nodes[node->index2]);
			query->n = (query->n - p1) / p2;
		}
	}

	// Instance shorthand
	const Instance& instance = assembly->instances[node->instance_index];

	// Get transforms if any
	if (instance.transform_count > 0) {
		auto cbegin = assembly->xforms.cbegin() + instance.transform_index;
		auto cend = cbegin + instance.transform_count;
		auto instance_xform = lerp_seq(query->time, cbegin, cend);
		query->pos = instance_xform.pos_to(query->pos);
		query->nor = instance_xform.nor_to(query->nor).normalized();
		query->xform *= instance_xform;
	}

	// Do light sampling
	if (instance.type == Instance::OBJECT) {
		const Object* obj = assembly->objects[instance.data_index].get(); // Shorthand

		if (obj->get_type() == Object::LIGHT) {
			const Light* light = dynamic_cast<const Light*>(obj);

			float p = 1.0f;
			query->color = light->sample(query->pos, query->u, query->v, query->time, &(query->to_light), &p);
			query->to_light = query->xform.dir_from(query->to_light);
			query->pdf *= (tot_prob * light_count()) * p;
		}
		// TODO: handle non-light objects that emit light
	} else if (instance.type == Instance::ASSEMBLY) {
		const Assembly* asmb = assembly->assemblies[instance.data_index].get(); // Shorthand

		query->pdf *= (tot_prob * light_count()) / asmb->light_accel.light_count();
		asmb->light_accel.sample(query);
	}
}


void LightTree::build(const Assembly& assembly_)
{
	assembly = &assembly_;

	// Populate the build nodes
	for (size_t i = 0; i < assembly->instances.size(); ++i) {
		const auto& instance = assembly->instances[i]; // Shorthand

		// If it's an object
		if (instance.type == Instance::OBJECT) {
			const Object* obj = assembly->objects[instance.data_index].get(); // Shorthand

			if (obj->total_emitted_color().energy() > 0.0f) {
				build_nodes.push_back(BuildNode());
				build_nodes.back().instance_index = i;
				build_nodes.back().bbox = assembly->instance_bounds_at(0.5f, i);
				build_nodes.back().center = build_nodes.back().bbox.center();
				const Vec3 scale = assembly->instance_xform_at(0.5f, i).get_inv_scale();
				const float surface_scale = ((scale[0]*scale[1]) + (scale[0]*scale[2]) + (scale[1]*scale[2])) * 0.33333333f;
				build_nodes.back().energy = obj->total_emitted_color().energy() / surface_scale;

				++total_lights;
			}
		}
		// If it's an assembly
		else if (instance.type == Instance::ASSEMBLY) {
			const Assembly* asmb = assembly->assemblies[instance.data_index].get(); // Shorthand
			const auto count = asmb->light_accel.light_count();
			const float energy = asmb->light_accel.total_emitted_color().energy();

			if (count > 0 && energy > 0.0f) {
				build_nodes.push_back(BuildNode());
				build_nodes.back().instance_index = i;
				if (instance.transform_count > 0) {
					auto xstart = assembly->xforms.cbegin() + instance.transform_index;
					auto xend = xstart + instance.transform_count;
					build_nodes.back().bbox = lerp_seq(0.5f, asmb->light_accel.bounds()).inverse_transformed(lerp_seq(0.5f, xstart, xend));
				} else {
					build_nodes.back().bbox = lerp_seq(0.5f, asmb->light_accel.bounds());
				}
				build_nodes.back().center = build_nodes.back().bbox.center();
				const Vec3 scale = assembly->instance_xform_at(0.5f, i).get_inv_scale();
				const float surface_scale = ((scale[0]*scale[1]) + (scale[0]*scale[2]) + (scale[1]*scale[2])) * 0.33333333f;
				build_nodes.back().energy = energy / surface_scale;

				total_lights += count;
			}
		}
	}

	if (build_nodes.size() > 0) {
		recursive_build(build_nodes.begin(), build_nodes.end());
		bounds_ = nodes[0].bounds;
		total_energy = nodes[0].energy;
	} else {
		bounds_.clear();
		bounds_.emplace_back(BBox());
	}
}


std::vector<LightTree::BuildNode>::iterator LightTree::split_lights(std::vector<LightTree::BuildNode>::iterator start, std::vector<LightTree::BuildNode>::iterator end)
{
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
	const float pmid = .5f * (min[max_axis] + max[max_axis]);
	auto mid_itr = std::partition(start, end, [max_axis, pmid](const BuildNode& bn) {
		return bn.center[max_axis] < pmid;
	});

	if (mid_itr == start)
		mid_itr++;

	return mid_itr;
}


size_t LightTree::recursive_build(std::vector<LightTree::BuildNode>::iterator start, std::vector<LightTree::BuildNode>::iterator end)
{
	// Allocate the node
	const size_t me = nodes.size();
	nodes.push_back(Node());

	if ((start + 1) == end) {
		// Leaf node
		const Instance& instance = assembly->instances[start->instance_index];

		nodes[me].is_leaf = true;
		nodes[me].instance_index = start->instance_index;

		// Get bounds
		if (instance.type == Instance::OBJECT) {
			nodes[me].bounds = assembly->instance_bounds(start->instance_index);
		} else if (instance.type == Instance::ASSEMBLY) {
			const Assembly* assmb = assembly->assemblies[instance.data_index].get();
			if (instance.transform_count > 0) {
				auto xstart = assembly->xforms.cbegin() + instance.transform_index;
				auto xend = xstart + instance.transform_count;
				nodes[me].bounds = transform_from(assmb->light_accel.bounds(), xstart, xend);
			} else {
				nodes[me].bounds = assmb->light_accel.bounds();
			}
		}

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
		nodes[me].bounds = merge(nodes[c1].bounds, nodes[c2].bounds);

		// Calculate energy
		nodes[me].energy = nodes[c1].energy + nodes[c2].energy;
	}

	return me;
}