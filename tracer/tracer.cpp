#include "tracer.hpp"

#include <limits>
#include <vector>
#include <iostream>
#include <algorithm>
#include <functional>
#include <assert.h>

#include "global.hpp"
#include "array.hpp"
#include "slice.hpp"
#include "counting_sort.hpp"
#include "low_level.hpp"

#include "micro_surface.hpp"
#include "micro_surface_cache.hpp"

#include "ray.hpp"
#include "intersection.hpp"
#include "scene.hpp"

#define RAY_STATE_SIZE scene->world.ray_state_size()
#define MAX_POTINT 1u
#define RAY_JOB_SIZE (1024*4)


uint32_t Tracer::trace(const Slice<Ray> rays_, Slice<Intersection> intersections_)
{
	Global::Stats::rays_shot += rays_.size();
	// Get rays
	rays.init_from(rays_);

	// Get and initialize intersections
	intersections.init_from(intersections_);
	std::fill(intersections.begin(), intersections.end(), Intersection());

	// Print number of rays being traced
	//std::cout << "\tTracing " << rays.size() << " rays" << std::endl;

	// Allocate and clear out ray states
	states.resize(rays.size()*RAY_STATE_SIZE);
	std::fill(states.begin(), states.end(), 0);

	// Allocate and init rays_active flags
	rays_active.resize(rays.size());
	std::fill(rays_active.begin(), rays_active.end(), true);

	// Allocate potential intersection buffer
	potential_intersections.resize(rays.size()*MAX_POTINT);

	// Trace potential intersections
	while (accumulate_potential_intersections()) {
		sort_potential_intersections();
		trace_potential_intersections();
	}

	return rays.size();
}


size_t Tracer::accumulate_potential_intersections()
{
	// Clear out potential intersection buffer
	potential_intersections.resize(rays.size()*MAX_POTINT);
	for (auto& potint: potential_intersections)
		potint.valid = false;

	// Trace scene acceleration structure to accumulate
	// potential intersections
	size_t potint_ids[MAX_POTINT];
	for (size_t i = 0; i < rays.size(); i++) {
		if (rays_active[i]) {
			const size_t pc = scene->world.get_potential_intersections(rays[i], intersections[i].t, MAX_POTINT, potint_ids, &(states[i*RAY_STATE_SIZE]));
			rays_active[i] = (pc > 0);

			for (size_t j = 0; j < pc; j++) {
				potential_intersections[(i*MAX_POTINT)+j].valid = true;
				potential_intersections[(i*MAX_POTINT)+j].object_id = potint_ids[j];
				potential_intersections[(i*MAX_POTINT)+j].ray_index = i;
			}
		}
	}

	// Compact the potential intersections
	size_t pii = 0;
	size_t last = 0;
	size_t i = 0;
	while (i < potential_intersections.size()) {
		while (last < potential_intersections.size() && potential_intersections[last].valid)
			last++;

		if (potential_intersections[i].valid) {
			pii++;
			if (i > last) {
				potential_intersections[last] = potential_intersections[i];
				potential_intersections[i].valid = false;
			}
		}

		i++;
	}
	potential_intersections.resize(pii);

	// Return the total number of potential intersections accumulated
	return pii;
}


/**
 * Uses a counting sort to sort the potential intersections by
 * object_id.
 */
void Tracer::sort_potential_intersections()
{
#if 1
	std::sort(potential_intersections.begin(), potential_intersections.end());
#else
	CountingSort::sort<PotentialInter>(&(*(potential_intersections.begin())),
	                                   potential_intersections.size(),
	                                   scene->world.max_primitive_id(),
	                                   &index_potint);
#endif
	return;
}


size_t Tracer::trace_diceable_surface(size_t potint_start)
{
	const size_t prim_id = potential_intersections[potint_start].object_id;
	DiceableSurfacePrimitive* primitive = dynamic_cast<DiceableSurfacePrimitive*>(&(scene->world.get_primitive(prim_id)));
	auto& bounds = primitive->bounds();
	std::shared_ptr<MicroSurface> micro_surface = MicroSurfaceCache::cache.get(primitive->uid);
	size_t current_subdivs;
	bool rediced = false; // Keeps track of whether we ever redice the surface

	if (micro_surface)
		current_subdivs = micro_surface->subdivisions();

	size_t i = potint_start;
	for (; i < potential_intersections.size() && potential_intersections[i].object_id == prim_id; ++i) {
		const auto& ray = rays[potential_intersections[i].ray_index];
		auto& intersection = intersections[potential_intersections[i].ray_index];

		// Get bounding box intersection
		float tnear, tfar;
		if (!bounds.intersect_ray(ray, &tnear, &tfar))
			continue;

		// Calculate the subdivision rate this ray needs for this primitive
		const float width = ray.min_width(tnear, tfar);
		const size_t subdivs = primitive->subdiv_estimate(width);

		// Figure out if we need to redice or not
		bool redice = false;
		if (micro_surface) {
			if (current_subdivs < subdivs)
				redice = true;
		} else {
			redice = true;
		}
		rediced = redice || rediced;

		// Redice if necessary
		if (redice) {
			micro_surface = std::shared_ptr<MicroSurface>(primitive->dice(subdivs));
			current_subdivs = subdivs;
			Global::Stats::cache_misses++;
		}

		// Trace the ray against the MicroSurface
		if (ray.is_shadow_ray) {
			if (!intersection.hit)
				intersection.hit |= micro_surface->intersect_ray(ray, width, nullptr);
			rays_active[potential_intersections[i].ray_index] = !intersection.hit; // Early out for shadow rays
		} else {
			intersection.hit |= micro_surface->intersect_ray(ray, width, &intersection);
		}
	}

	// If we created a new microsurface, store it in the cache
	if (rediced)
		MicroSurfaceCache::cache.put(micro_surface, primitive->uid);

	return i;
}



void Tracer::trace_potential_intersections()
{
	for (size_t i = 0; i < potential_intersections.size(); i++) {
		// Prefetch memory for next iteration, to hide memory latency
		//prefetch_L3(&(potential_intersections[i+2]));
		//prefetch_L3(&(rays[potential_intersections[i+1].ray_index]));
		//prefetch_L3(&(intersections[potential_intersections[i+1].ray_index]));

		// Shorthand references
		//auto& primitive = scene->world.get_primitive(potential_intersections[i].object_id);

		i = trace_diceable_surface(i) - 1;
	}

	Global::Stats::primitive_ray_tests += potential_intersections.size();
}



