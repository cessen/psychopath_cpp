#include "tracer.hpp"

#include <vector>
#include <assert.h>
#include <boost/thread.hpp>

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

void Tracer::tracey(uint32 start, uint32 end)
{
	for (uint32 i = start; i < end; i++) {
		if (rayinters[i]->ray.is_shadow_ray)
			rayinters[i]->hit = scene->intersect_ray(rayinters[i]->ray, NULL);
		else
			rayinters[i]->hit = scene->intersect_ray(rayinters[i]->ray, &(rayinters[i]->inter));
	}
}

uint32 Tracer::trace_rays()
{
	uint32 s = rayinters.size();

	boost::thread traceys[thread_count];

	if (s >= (uint32)(thread_count)) {
		// Start threads
		for (int i=0; i < thread_count; i++) {
			uint32 start = (s*i) / thread_count;
			uint32 end   = (s*(i+1)) / thread_count;

			traceys[i] = boost::thread(&Tracer::tracey, this, start, end);
		}

		// Join threads
		for (int i=0; i < thread_count; i++) {
			traceys[i].join();
		}
	} else {
		tracey(0, s);
	}

	rayinters.clear();
	return s;
}

