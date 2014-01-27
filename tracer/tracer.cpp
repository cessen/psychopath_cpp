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
				potential_intersections[(i*MAX_POTINT)+j].nearest_hit_t = intersections[i].t;
			}
		}
	}

	// Compact the potential intersections to only the valid ones
	const auto last = std::partition(potential_intersections.begin(), potential_intersections.end(), [](const PotentialInter& p) {
		return p.valid;
	});
	size_t potint_count = std::distance(potential_intersections.begin(), last);
	potential_intersections.resize(potint_count);

	// Sort potential intersections by primitive id
	std::sort(potential_intersections.begin(), potential_intersections.end());

	// Return the total number of potential intersections accumulated
	return potint_count;
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

	// Traversal
	while (stack_i >= 0) {
		DiceableSurfacePrimitive& primitive = *(primitive_stack[stack_i]); // Shorthand reference to potint's primitive

		// Fetch the current primitive's microgeo cache if it exists
		std::shared_ptr<MicroSurface> micro_surface = cache.get(Key(uid1, uid2_stack[stack_i]));
		bool rediced = false;

		// Get the number of subdivisions of the cached microsurface if it exists
		size_t current_subdivs = 0;
		if (micro_surface)
			current_subdivs = micro_surface->subdivisions();

		// Test potints against primitive, marking for deeper traversal
		// if they can't be directly tested
		for (auto pitr = potint_starts[stack_i]; pitr != potint_ends[stack_i]; ++pitr) {
			// Prefetch three rays ahead to keep ahead of memory latency
			if ((pitr+3) < potint_ends[stack_i])
				LowLevel::prefetch_L1(&(rays[(pitr+3)->ray_index]));

			// Setup
			pitr->tag = 0; // No traversing deeper by default
			const Ray& ray = rays[pitr->ray_index];  // Shorthand reference to potint's ray
			Intersection& inter = intersections[pitr->ray_index]; // Shorthand reference to potint's intersection

			// If the potint intersects with the primitive's bbox
			float tnear, tfar;
			if (primitive.bounds().intersect_ray(ray, &tnear, &tfar, pitr->nearest_hit_t)) {
				// Calculate the width of the ray within the bounding box
				const float width = ray.min_width(tnear, tfar);

				// Calculate the number of subdivisions necessary for this ray
				const size_t subdivs = primitive.subdiv_estimate(width);

				// If it's under the max subdivisions allowed, test
				// against primitive
				if (subdivs <= max_subdivs) {
					// If we're missing a cached microsurface or it's not high resolution enough,
					// dice a new one
					if (micro_surface == nullptr || subdivs > current_subdivs) {
						micro_surface = primitive.dice(subdivs);
						current_subdivs = subdivs;
						rediced = true;
					}

					// Test against the ray
					if (ray.is_shadow_ray) {
						inter.hit |= micro_surface->intersect_ray(ray, width, nullptr);
						rays_active[pitr->ray_index] = !inter.hit; // Early out for shadow rays
					} else {
						inter.hit |= micro_surface->intersect_ray(ray, width, &inter);
						pitr->nearest_hit_t = inter.t;
					}
				}
				// If it's over the max subdivisions allowed, mark for deeper traversal
				else {
					pitr->tag = 1;
				}
			}
		} // End test potints

		// If we re-diced the microsurface, store it in the cache
		if (rediced)
			cache.put(micro_surface, Key(uid1, uid2_stack[stack_i]));

		// Filter potints based on whether they need deeper traversal
		potint_starts[stack_i] = std::partition(potint_starts[stack_i], potint_ends[stack_i], [this](const PotentialInter& p) {
			return p.tag == 0 || this->rays_active[p.ray_index] == false;
		});

		// If any potints left, traverse down the stack via splitting
		if (potint_starts[stack_i] != potint_ends[stack_i]) {
			++split_count;
			std::unique_ptr<DiceableSurfacePrimitive> new_prims[4];

			// Split the primitive
			const int new_count = primitive.split(new_prims);

			const auto parent_uid2 = uid2_stack[stack_i];
			for (int i = 0; i < new_count; ++i) {
				const int ii = stack_i + i;

				// Update potint iterator stack
				potint_starts[ii] = potint_starts[stack_i];
				potint_ends[ii] = potint_ends[stack_i];

				// Update primitive stack
				std::swap(primitive_stack[ii], new_prims[i]);

				// Update uid stack
				uid2_stack[ii] = (parent_uid2 << 2) | i;
			}

			// Update stack index_potint
			stack_i += new_count - 1;
		}
		// If not any potints left, move up the stack
		else {
			stack_i--;
		}
	}

	Global::Stats::split_count += split_count;

	return start + potint_count;
}



void Tracer::trace_potential_intersections()
{
	for (auto itr = potential_intersections.begin(); itr != potential_intersections.end(); ++itr) {
		// Shorthand references
		//auto& primitive = scene->world.get_primitive(potential_intersections[i].object_id);

		itr = trace_diceable_surface(itr, potential_intersections.end()) - 1;
	}

	Global::Stats::primitive_ray_tests += potential_intersections.size();
}



