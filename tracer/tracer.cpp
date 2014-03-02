#include "tracer.hpp"

#include <limits>
#include <cmath>
#include <vector>
#include <tuple>
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


uint32_t Tracer::trace(const Slice<WorldRay> w_rays_, Slice<Intersection> intersections_)
{
	Global::Stats::rays_shot += w_rays_.size();

	// Get rays
	w_rays.init_from(w_rays_);

	// Get and initialize intersections
	intersections.init_from(intersections_);
	std::fill(intersections.begin(), intersections.end(), Intersection());

	// Initialize traverser
	traverser.init_rays(w_rays.begin(), w_rays.end());

	// Trace potential intersections
	std::tuple<Ray*, Ray*, Object*> hits = traverser.next_object();
	while (std::get<2>(hits) != nullptr) {
		// Branch to different code path based on object type
		switch (std::get<2>(hits)->get_type()) {
			case Object::SURFACE:
				trace_surface(reinterpret_cast<Surface*>(std::get<2>(hits)), std::get<0>(hits), std::get<1>(hits));
				break;
			case Object::DICEABLE_SURFACE:
				trace_diceable_surface(reinterpret_cast<DiceableSurface*>(std::get<2>(hits)), std::get<0>(hits), std::get<1>(hits));
				break;
			default:
				std::cout << "WARNING: unknown object type, skipping." << std::endl;
				break;
		}

		hits = traverser.next_object();
	}

	return w_rays.size();
}



void Tracer::trace_surface(Surface* surface, Ray* rays, Ray* end)
{
	for (auto ritr = rays; ritr != end; ++ritr) {
		Ray& ray = *ritr;  // Shorthand reference to the ray
		Intersection& inter = intersections[ritr->id]; // Shorthand reference to ray's intersection

		// Test against the ray
		inter.hit |= surface->intersect_ray(ray, &inter);
		if (inter.hit) {
			if (ray.type == Ray::OCCLUSION) {
				ray.flags |= Ray::DONE; // Early out for shadow rays
			} else {
				ray.max_t = inter.t;
			}
		}
	}
}



void Tracer::trace_diceable_surface(DiceableSurface* prim, Ray* rays, Ray* end)
{
#define STACK_SIZE 32

	using namespace MicroSurfaceCache;
	int split_count = 0;

	const size_t max_subdivs = intlog2(Config::max_grid_size);

	// UID's
	const size_t uid1 = prim->uid; // Main UID
	size_t uid2_stack[STACK_SIZE]; // Sub-UID
	uid2_stack[0] = 1;

	// Stack
	std::unique_ptr<DiceableSurface> primitive_stack[STACK_SIZE];
	primitive_stack[0] = prim->copy();
	int stack_i = 0;

	// Stacks of start/end iterators for partitioning the potints as we
	// dive deeper into the traversal
	Ray* ray_starts[STACK_SIZE];
	Ray* ray_ends[STACK_SIZE];
	ray_starts[0] = rays;
	ray_ends[0] = end;

	// Traversal
	while (stack_i >= 0) {
		DiceableSurface& primitive = *(primitive_stack[stack_i]); // Shorthand reference to potint's primitive

		// Fetch the current primitive's microgeo cache if it exists
		std::shared_ptr<MicroSurface> micro_surface = cache.get(Key(uid1, uid2_stack[stack_i]));
		bool rediced = false;

		// Get the number of subdivisions of the cached microsurface if it exists
		size_t current_subdivs = 0;
		if (micro_surface)
			current_subdivs = micro_surface->subdivisions();

		// Test rays against primitive, marking for deeper traversal
		// if they can't be directly tested
		for (auto ritr = ray_starts[stack_i]; ritr != ray_ends[stack_i]; ++ritr) {
			// Setup
			ritr->flags &= ~Ray::DEEPER_SPLIT; // No traversing deeper by default
			Ray& ray = *ritr;  // Shorthand reference to the ray
			Intersection& inter = intersections[ritr->id]; // Shorthand reference to ray's intersection

			// If the potint intersects with the primitive's bbox
			float tnear, tfar;
			if (primitive.bounds().intersect_ray(ray, &tnear, &tfar, ray.max_t)) {
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
					inter.hit |= micro_surface->intersect_ray(ray, width, &inter);
					if (inter.hit) {
						if (ray.type == Ray::OCCLUSION) {
							ray.flags |= Ray::DONE; // Early out for shadow rays
						} else {
							ray.max_t = inter.t;
						}
					}
				}
				// If it's over the max subdivisions allowed, mark for deeper traversal
				else {
					ray.flags |= Ray::DEEPER_SPLIT;
				}
			}
		} // End test potints

		// If we re-diced the microsurface, store it in the cache
		if (rediced)
			cache.put(micro_surface, Key(uid1, uid2_stack[stack_i]));

		// Filter potints based on whether they need deeper traversal
		ray_starts[stack_i] = std::partition(ray_starts[stack_i], ray_ends[stack_i], [this](const Ray& r) {
			return ((r.flags & Ray::DEEPER_SPLIT) == 0) || ((r.flags & Ray::DONE) != 0);
		});

		// If any potints left, traverse down the stack via splitting
		if (ray_starts[stack_i] != ray_ends[stack_i]) {
			++split_count;
			std::unique_ptr<DiceableSurface> new_prims[4];

			// Split the primitive
			const int new_count = primitive.split(new_prims);

			const auto parent_uid2 = uid2_stack[stack_i];
			for (int i = 0; i < new_count; ++i) {
				const int ii = stack_i + i;

				// Update potint iterator stack
				ray_starts[ii] = ray_starts[stack_i];
				ray_ends[ii] = ray_ends[stack_i];

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
}



