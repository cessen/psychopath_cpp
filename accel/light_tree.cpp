#include "light_tree.hpp"

#include "assembly.hpp"

void LightTree::build(const Assembly& assembly)
{
	// Populate the build nodes
	for (const auto& object: assembly.objects) {
		if (object->get_type() == Object::LIGHT) {
			Light* light = dynamic_cast<Light*>(object.get());

			build_nodes.push_back(BuildNode());
			build_nodes.back().light = light;
			build_nodes.back().bbox = light->bounds()[0]; // TODO: use full std::vector<BBox> for time
			build_nodes.back().center = build_nodes.back().bbox.center();
			build_nodes.back().energy = light->total_energy();
		}
	}

	if (build_nodes.size() > 0)
		recursive_build(build_nodes.begin(), build_nodes.end());
}
