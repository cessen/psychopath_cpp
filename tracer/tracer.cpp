#include "tracer.hpp"

#include <vector>
#include <iostream>
#include <assert.h>
#include <boost/thread.hpp>
#include "array.hpp"

#include "ray.hpp"
#include "scene.hpp"
#include "rayinter.hpp"


uint32 Tracer::queue_rays(const Array<RayInter *> &rayinters_)
{
	const uint32 size = rayinters.size() + rayinters_.size();
	rayinters.reserve(size);

	uint32 s = rayinters_.size();
	for (uint32 i = 0; i < s; i++) {
		rayinters.push_back(rayinters_[i]);
	}
	return size;
}

#define MAX_POTINT 8

void Tracer::tracey(uint32 start, uint32 end)
{
	uint32 potints[MAX_POTINT];
	uint32 potint_count;
	uint64 state[2];

	for (uint32 i = start; i < end; i++) {
		state[0] = 0;
		state[1] = 0;
		bool shadow_hit = false;

		do {
			potint_count = scene->world.get_potential_intersections(rayinters[i]->ray, MAX_POTINT, potints, state);

			for (uint32 i2 = 0; i2 < potint_count; i2++) {
				if (rayinters[i]->ray.is_shadow_ray) {
					rayinters[i]->hit |= scene->world.get_primitive(potints[i2]).intersect_ray(rayinters[i]->ray, NULL);

					// Early out for shadow rays
					if (rayinters[i]->hit) {
						shadow_hit = true;
						break;
					}
				} else
					rayinters[i]->hit |= scene->world.get_primitive(potints[i2]).intersect_ray(rayinters[i]->ray, &(rayinters[i]->inter));
			}
		} while (potint_count > 0 && !shadow_hit);
	}
}

uint32 Tracer::trace_rays()
{
	uint32 s = rayinters.size();
	std::cout << "\tTracing " << s << " rays" << std::endl;

	boost::thread traceys[thread_count];

	if (s >= (uint32)(thread_count) && (thread_count > 1)) {
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

