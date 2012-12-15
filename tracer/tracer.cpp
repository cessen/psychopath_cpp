#include "tracer.hpp"

#include <vector>
#include <iostream>
#include <algorithm>
#include <assert.h>
#include <boost/thread.hpp>
#include "array.hpp"

#include "ray.hpp"
#include "scene.hpp"
#include "rayinter.hpp"


uint32 Tracer::queue_rays(const Array<RayInter *> &ray_inters_)
{
	const uint32 size = ray_inters.size() + ray_inters_.size();
	ray_inters.reserve(size);

	uint32 s = ray_inters_.size();
	for (uint32 i = 0; i < s; i++) {
		ray_inters.push_back(ray_inters_[i]);
	}
	return size;
}

#define MAX_POTINT 1

bool compare_potint(const PotentialInter &a, const PotentialInter &b)
{
	// Empty potential intersections always go after non-emtpy
	if (a.ray_inter != NULL && b.ray_inter == NULL)
		return true;
	else if (a.ray_inter == NULL)
		return false;

	// Sort by object id
	if (a.object_id < b.object_id)
		return true;
	else
		return false;
}

uint64 Tracer::accumulate_potential_intersections()
{
	// TODO: make multi-threaded

	// Clear out potential intersection buffer
	potential_inters.resize(potential_inters.capacity());
	const uint32 spi = potential_inters.size();
	for (uint32 i = 0; i < spi; i++)
		potential_inters[i].ray_inter = NULL;

	// Accumulate potential intersections
	uint64 pii = 0;
	uint_i potint_ids[MAX_POTINT];
	const uint64 sri = ray_inters.size();
	for (uint64 i = 0; i < sri; i++) {
		const uint32 potint_count = scene->world.get_potential_intersections(ray_inters[i]->ray, MAX_POTINT, potint_ids, &(states[i*2]));
		for (uint32 j = 0; j < potint_count; j++) {
			potential_inters[pii+j].object_id = potint_ids[j];
			potential_inters[pii+j].ray_inter = ray_inters[i];
		}
		pii += potint_count;
	}

	// Minimize and sort the potential intersections
	potential_inters.resize(pii);
	if (potential_inters.size() > 0)
		std::sort(potential_inters.begin(), potential_inters.end(), compare_potint);

	// Return the total number of potential intersections accumulated
	return pii;
}

void Tracer::trace_potential_intersections()
{
	const uint64 spi = potential_inters.size();

	for (uint32 i = 0; i < spi; i++) {
		RayInter *ray_inter = potential_inters[i].ray_inter;
		uint64 id = potential_inters[i].object_id;

		if (ray_inter->ray.is_shadow_ray) {
			if (!ray_inter->hit)
				ray_inter->hit |= scene->world.get_primitive(id).intersect_ray(ray_inter->ray, NULL);
		} else {
			ray_inter->hit |= scene->world.get_primitive(id).intersect_ray(ray_inter->ray, &(ray_inter->inter));
		}
	}
}

uint32 Tracer::trace_rays()
{
	uint32 s = ray_inters.size();
	std::cout << "\tTracing " << s << " rays" << std::endl;

	// Allocate and clear out ray states
	states.resize(s*2);
	for (uint_i i = 0; i < s*2; i++)
		states[i] = 0;

	// Allocate potential intersection buffer
	potential_inters.resize(s*MAX_POTINT);

	/*boost::thread traceys[thread_count];

	if (s >= (uint32)(thread_count) && (thread_count > 1)) {
		// Start threads
		for (int i=0; i < thread_count; i++) {
			uint32 start = (s*i) / thread_count;
			uint32 end   = (s*(i+1)) / thread_count;

			traceys[i] = boost::thread(&Tracer::accumulate_potential_intersections, this);
		}

		// Join threads
		for (int i=0; i < thread_count; i++) {
			traceys[i].join();
		}
	} else {
		accumulate_potential_intersections();
	}*/
	while (accumulate_potential_intersections()) {
		trace_potential_intersections();
	}

	ray_inters.clear();
	return s;
}

