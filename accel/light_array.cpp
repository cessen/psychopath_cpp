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
	float p = 1.0f;
	const float locals = static_cast<double>(light_indices.size()) / (total_assembly_lights + light_indices.size());

	// If we're sampling a light in this assembly
	if (query->n <= locals) {
		// Update probabilities and get light index
		query->n /= locals;
		p = (p * locals) / light_indices.size();
		const auto index = light_indices[static_cast<uint32_t>(query->n * light_indices.size()) % light_indices.size()];

		const Instance& instance = assembly->instances[index]; // Shorthand

		// Get the light
		Light* light = dynamic_cast<Light*>(assembly->objects[instance.data_index].get());

		// Sample the light
		if (instance.transform_count == 0) {
			query->color = light->sample(query->pos, query->u, query->v, query->time, &(query->to_light), &(query->pdf));
			query->pdf *= p;
		} else {
			auto cbegin = assembly->xforms.cbegin() + instance.transform_index;
			auto cend = cbegin + instance.transform_count;
			Transform xform = lerp_seq(query->time, cbegin, cend);
			query->color = light->sample(xform.pos_from(query->pos), query->u, query->v, query->time, &(query->to_light), &(query->pdf));
			query->to_light = xform.dir_to(query->to_light);
			query->pdf *= p;
		}
	}
	// If we're sampling a light in a child assembly
	else {

	}
}
