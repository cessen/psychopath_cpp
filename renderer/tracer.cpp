#include "tracer.hpp"

#include <vector>
#include <assert.h>

#include "ray.hpp"
#include "scene.hpp"
#include "rayinter.hpp"

uint32 Tracer::queue_rays(const std::vector<RayInter *> &rayinters_)
{
	const uint32 size = rayinters.size() + rayinters_.size();
	rayinters.reserve(size);

	uint32 s = rayinters_.size();
	for (uint32 i = 0; i < s; i++) {
		rayinters.push_back(rayinters_[i]);
	}

	return size;
}

uint32 Tracer::trace_rays()
{
	uint32 s = rayinters.size();
	for (uint32 i = 0; i < s; i++) {
		rayinters[i]->hit = scene->intersect_ray(rayinters[i]->ray, &(rayinters[i]->inter));
	}

	rayinters.clear();
	return s;
}

