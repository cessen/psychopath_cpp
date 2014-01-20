#include "tracer.hpp"

#include <limits>
#include <cmath>
#include <vector>
#include <iostream>
#include <algorithm>
#include <functional>
#include <assert.h>

#include "global.hpp"
#include "array.hpp"
#include "slice.hpp"
#include "counting_sort.hpp"
#include "utils.hpp"
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


std::vector<PotentialInter>::iterator Tracer::trace_diceable_surface(std::vector<PotentialInter>::iterator start, std::vector<PotentialInter>::iterator end)
{
#define STACK_SIZE 32
	using namespace MicroSurfaceCache;
	int split_count = 0;

	const size_t max_subdivs = intlog2(Config::max_grid_size);
	const size_t prim_id = start->object_id;

	// UID's
	const size_t uid1 = scene->world.get_primitive(prim_id).uid; // Main UID
	size_t uid2_stack[STACK_SIZE]; // Sub-UID
	uid2_stack[0] = 1;

	// Stack
	std::unique_ptr<DiceableSurfacePrimitive> primitive_stack[STACK_SIZE];
	primitive_stack[0] = (dynamic_cast<DiceableSurfacePrimitive*>(&(scene->world.get_primitive(prim_id)))->copy());
	int stack_i = 0;

	// Find out how many potints we're dealing with
	int potint_count = 0;
	for (auto itr = start; (itr != end) && (itr->object_id == prim_id); ++itr)
		++potint_count;

	// Stacks of start/end iterators for partitioning the potints as we
	// dive deeper into the traversal
	std::vector<PotentialInter>::iterator potint_starts[STACK_SIZE];
	std::vector<PotentialInter>::iterator potint_ends[STACK_SIZE];
	potint_starts[0] = start;
	potint_ends[0] = start + potint_count;

	// A place to keep our microsurfaces during traversal
	std::shared_ptr<MicroSurface> micro_surface;

	// Traversal
	while (stack_i >= 0) {
		auto& bounds = primitive_stack[stack_i]->bounds();
		size_t current_subdivs;
		bool rediced = false; // Keeps track of whether we end up redicing the surface
		bool split = false; // Keeps track of whether we need to split and traverse further

		// Fetch a cached microsurface if there is one
		micro_surface = cache.get(Key(uid1, uid2_stack[stack_i]));
		if (micro_surface)
			current_subdivs = micro_surface->subdivisions();

		// Trace the rays against the current stack primitive
		for (auto pitr = potint_starts[stack_i]; pitr != potint_ends[stack_i]; ++pitr) {
			prefetch_L3(&(pitr[2]));
			prefetch_L3(&(rays[pitr[1].ray_index]));
			prefetch_L3(&(intersections[pitr[1].ray_index]));
			prefetch_L3(&(rays_active[pitr->ray_index]));

			const auto& ray = rays[pitr->ray_index];
			auto& intersection = intersections[pitr->ray_index];
			pitr->tag = 0;

			// Get bounding box intersection
			float tnear, tfar;
			if (!bounds.intersect_ray(ray, &tnear, &tfar))
				continue; // Continue to next ray if we didn't hit

			// Calculate the subdivision rate this ray needs for this primitive
			const float width = ray.min_width(tnear, tfar);
			size_t subdivs = primitive_stack[stack_i]->subdiv_estimate(width);

			// If we need more resolution for this ray, mark for further downward traversal
			if (subdivs > max_subdivs) {
				pitr->tag = 1;
				split = true;
				continue; // Continue to next ray
			}

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
				micro_surface = std::shared_ptr<MicroSurface>(primitive_stack[stack_i]->dice(subdivs));
				current_subdivs = subdivs;
				Global::Stats::cache_misses++;
			}

			// Trace the ray against the MicroSurface
			if (ray.is_shadow_ray) {
				if (!intersection.hit)
					intersection.hit |= micro_surface->intersect_ray(ray, width, nullptr);
				rays_active[pitr->ray_index] = !intersection.hit; // Early out for shadow rays
			} else {
				intersection.hit |= micro_surface->intersect_ray(ray, width, &intersection);
			}
		}

		// If we created a new microsurface while testing against the rays, store it in the cache
		if (rediced)
			cache.put(micro_surface, Key(uid1, uid2_stack[stack_i]));

		// Split and traverse further down if needed
		if (split) {
			++split_count;
			// Split primitive
			const unsigned int new_count = primitive_stack[stack_i]->split(&(primitive_stack[stack_i]));
			const int new_stack_i = stack_i + new_count - 1;

			// Update potint iterator stacks
			potint_starts[stack_i] = std::partition(potint_starts[stack_i], potint_ends[stack_i], [](const PotentialInter& p) {
				return p.tag == 0;
			});
			for (int i = 1; i < new_count; ++i) {
				potint_starts[stack_i + i] = potint_starts[stack_i];
				potint_ends[stack_i + i] = potint_ends[stack_i];
			}

			// Update uid stack
			auto parent_uid2 = uid2_stack[stack_i];
			for (unsigned int i = 0; i < new_count; ++i)
				uid2_stack[stack_i + i] = (parent_uid2 << 3) | (i + 1);

			stack_i = new_stack_i;
			continue;
		}

		// Traverse back up
		stack_i--;
	}

	Global::Stats::split_count += split_count;

	return start + potint_count;
}



void Tracer::trace_potential_intersections()
{
	for (auto itr = potential_intersections.begin(); itr != potential_intersections.end(); ++itr) {
		// Prefetch memory for next iteration, to hide memory latency
		//prefetch_L3(&(potential_intersections[i+2]));
		//prefetch_L3(&(rays[potential_intersections[i+1].ray_index]));
		//prefetch_L3(&(intersections[potential_intersections[i+1].ray_index]));

		// Shorthand references
		//auto& primitive = scene->world.get_primitive(potential_intersections[i].object_id);

		itr = trace_diceable_surface(itr, potential_intersections.end()) - 1;
	}

	Global::Stats::primitive_ray_tests += potential_intersections.size();
}



