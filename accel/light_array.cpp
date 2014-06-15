#include "light_array.hpp"

#include "assembly.hpp"

void LightArray::build(const Assembly& assembly_)
{
	assembly = &assembly_;

	for (size_t i = 0; i < assembly->instances.size(); ++i) {
		const auto& instance = assembly->instances[i]; // Shorthand
		if (instance.type == Instance::OBJECT) {
			if (assembly->objects[instance.data_index]->get_type() == Object::LIGHT)
				light_indices.push_back(i);
		} else if (instance.type == Instance::ASSEMBLY) {
			const auto count = assembly->assemblies[instance.data_index]->light_accel.light_count();
			if (count > 0) {
				assembly_lights.emplace_back(total_assembly_lights, count, i);
				total_assembly_lights += count;
			}
		}
	}
}



void LightArray::sample(LightQuery* query)
{
	// Handle empty light accel
	if (light_indices.size() == 0 && assembly_lights.size() == 0) {
		query->color = Color(0.0f);
		return;
	}

	const float local_prob = static_cast<double>(light_indices.size()) / (total_assembly_lights + light_indices.size());
	const float child_prob = 1.0f - local_prob;

	// If we're sampling a light in this assembly
	if (query->n <= local_prob) {
		// Update probabilities
		query->n /= local_prob;
		query->pdf = (query->pdf * local_prob) / light_indices.size();

		// Get light instance
		const auto index = light_indices[static_cast<uint32_t>(query->n * light_indices.size()) % light_indices.size()];
		const Instance& instance = assembly->instances[index]; // Shorthand

		// Get light data
		Light* light = dynamic_cast<Light*>(assembly->objects[instance.data_index].get());

		// Get transforms if any
		if (instance.transform_count > 0) {
			auto cbegin = assembly->xforms.cbegin() + instance.transform_index;
			auto cend = cbegin + instance.transform_count;
			query->xform *= lerp_seq(query->time, cbegin, cend);
		}

		// Sample the light
		float p;
		query->color = light->sample(query->xform.pos_from(query->pos), query->u, query->v, query->time, &(query->to_light), &p);
		query->to_light = query->xform.dir_to(query->to_light);
		query->pdf *= p;
	}
	// If we're sampling a light in a child assembly
	else {
		// Update probabilities
		query->n = (query->n - local_prob) / child_prob;
		query->pdf = (query->pdf * child_prob) / total_assembly_lights;

		// Select assembly
		// TODO: a binary search would be faster
		size_t index = 0;
		const size_t target_index = static_cast<size_t>(total_assembly_lights * query->n) % total_assembly_lights;
		for (const auto& al: assembly_lights) {
			if (std::get<0>(al) <= target_index && target_index < (std::get<0>(al) + std::get<1>(al))) {
				index = std::get<2>(al);
				query->pdf *= std::get<1>(al); // Update probability with the number of lights in the child assembly
				break;
			}
		}

		// Get assembly instance shorthand
		const Instance& instance = assembly->instances[index];

		// Get assembly
		Assembly* child_assembly = assembly->assemblies[instance.data_index].get();

		// Get transforms if any
		if (instance.transform_count > 0) {
			auto cbegin = assembly->xforms.cbegin() + instance.transform_index;
			auto cend = cbegin + instance.transform_count;
			query->xform *= lerp_seq(query->time, cbegin, cend);
		}

		// Traverse into child assembly
		child_assembly->light_accel.sample(query);
	}
}
