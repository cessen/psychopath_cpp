#include "light_array.hpp"

#include "assembly.hpp"

void LightArray::build(const Assembly& assembly)
{
	for (const auto& object: assembly.objects) {
		if (object->get_type() == Object::LIGHT)
			lights.push_back(dynamic_cast<Light*>(object.get()));
	}
}
