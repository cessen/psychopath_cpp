#include "tracer.hpp"

#include <iostream>
#include <limits>
#include <cmath>
#include <vector>
#include <tuple>
#include <iostream>
#include <algorithm>
#include <functional>
#include <assert.h>

#include "global.hpp"
#include "counting_sort.hpp"
#include "utils.hpp"
#include "low_level.hpp"

#include "micro_surface.hpp"
#include "micro_surface_cache.hpp"

#include "ray.hpp"
#include "intersection.hpp"
#include "assembly.hpp"
#include "scene.hpp"


uint32_t Tracer::trace(const WorldRay* w_rays_begin, const WorldRay* w_rays_end, Intersection* intersections_begin, Intersection* intersections_end)
{
	// Get rays
	w_rays = make_range(w_rays_begin, w_rays_end);
	Global::Stats::rays_shot += w_rays.size();

	// Create initial rays
	rays.resize(w_rays.size());
	for (size_t i = 0; i < rays.size(); ++i) {
		rays[i] = w_rays[i].to_ray();
		rays[i].id = i;
	}

	// Get and initialize intersections
	intersections = make_range(intersections_begin, intersections_end);
	std::fill(intersections.begin(), intersections.end(), Intersection());

	// Start tracing!
	std::vector<Transform> xforms(0);

#if 0
	// Split rays into groups based on primary direction before tracing
	auto split1 = std::partition(rays.begin(), rays.end(), [](Ray r) {
		return r.d[0] > 0 &&
		       std::abs(r.d[0]) > std::abs(r.d[1]) &&
		       std::abs(r.d[0]) > std::abs(r.d[2]);
	});
	trace_assembly(scene->root.get(), xforms, &(*rays.begin()), &(*split1));

	auto split2 = std::partition(split1, rays.end(), [](Ray r) {
		return r.d[0] <= 0 &&
		       std::abs(r.d[0]) > std::abs(r.d[1]) &&
		       std::abs(r.d[0]) > std::abs(r.d[2]);
	});
	trace_assembly(scene->root.get(), xforms, &(*split1), &(*split2));

	auto split3 = std::partition(split2, rays.end(), [](Ray r) {
		return r.d[1] > 0 &&
		       std::abs(r.d[1]) > std::abs(r.d[2]);
	});
	trace_assembly(scene->root.get(), xforms, &(*split2), &(*split3));

	auto split4 = std::partition(split3, rays.end(), [](Ray r) {
		return r.d[1] <= 0 &&
		       std::abs(r.d[1]) > std::abs(r.d[2]);
	});
	trace_assembly(scene->root.get(), xforms, &(*split3), &(*split4));

	auto split5 = std::partition(split4, rays.end(), [](Ray r) {
		return r.d[2] > 0;
	});
	trace_assembly(scene->root.get(), xforms, &(*split4), &(*split5));

	trace_assembly(scene->root.get(), xforms, &(*split5), &(*rays.end()));
#else
	// Just trace all the rays together
	trace_assembly(scene->root.get(), xforms, &(*rays.begin()), &(*rays.end()));
#endif

	return w_rays.size();
}



void Tracer::trace_assembly(Assembly* assembly, const std::vector<Transform>& parent_xforms, Ray* rays, Ray* rays_end)
{
	BVH2StreamTraverser traverser;

	// Initialize traverser
	traverser.init_accel(assembly->object_accel);
	traverser.init_rays(rays, rays_end);

	// Trace rays one object at a time
	std::tuple<Ray*, Ray*, size_t> hits = traverser.next_object();
	while (std::get<0>(hits) != std::get<1>(hits)) {
		const auto& instance = assembly->instances[std::get<2>(hits)]; // Short-hand for the current instance

		// Propagate transforms
		const auto xbegin = assembly->xforms.begin() + instance.transform_index;
		const auto xend = xbegin + instance.transform_count;
		std::vector<Transform> xforms {merge(parent_xforms.begin(), parent_xforms.end(), xbegin, xend)};

		// Transform rays if necessary
		if (instance.transform_count > 0) {
			for (auto ray = std::get<0>(hits); ray != std::get<1>(hits); ++ray) {
				w_rays[ray->id].update_ray(ray, lerp_seq(ray->time, xforms.begin(), xforms.end()));
			}
		}

		// Trace against the object or assembly
		if (instance.type == Instance::OBJECT) {
			Object* obj = assembly->objects[instance.data_index].get(); // Short-hand for the current object

			// Branch to different code path based on object type
			switch (obj->get_type()) {
				case Object::SURFACE:
					trace_surface(reinterpret_cast<Surface*>(obj), xforms, std::get<0>(hits), std::get<1>(hits));
					break;
				case Object::DICEABLE_SURFACE:
					trace_diceable_surface(reinterpret_cast<DiceableSurface*>(obj), xforms, std::get<0>(hits), std::get<1>(hits));
					break;
				default:
					std::cout << "WARNING: unknown object type, skipping." << std::endl;
					break;
			}

			Global::Stats::object_ray_tests += std::distance(std::get<0>(hits), std::get<1>(hits));
		} else { /* Instance::ASSEMBLY */
			Assembly* asmb = assembly->assemblies[instance.data_index].get(); // Short-hand for the current object
			trace_assembly(asmb, xforms, std::get<0>(hits), std::get<1>(hits));
		}

		// Un-transform rays if we transformed them earlier
		if (instance.transform_count > 0) {
			if (parent_xforms.size() > 0) {
				auto xbegin = parent_xforms.cbegin();
				auto xend = parent_xforms.cend();
				for (auto ray = std::get<0>(hits); ray != std::get<1>(hits); ++ray) {
					w_rays[ray->id].update_ray(ray, lerp_seq(ray->time, xbegin, xend));
				}
			} else {
				for (auto ray = std::get<0>(hits); ray != std::get<1>(hits); ++ray) {
					w_rays[ray->id].update_ray(ray);
				}
			}
		}

		// Get next object to test against
		hits = traverser.next_object();
	}
}



void Tracer::trace_surface(Surface* surface, const std::vector<Transform>& parent_xforms, Ray* rays, Ray* end)
{
	for (auto ritr = rays; ritr != end; ++ritr) {
		Ray& ray = *ritr;  // Shorthand reference to the ray
		Intersection& inter = intersections[ritr->id]; // Shorthand reference to ray's intersection

		// Test against the ray
		if (surface->intersect_ray(ray, &inter)) {
			inter.hit = true;

			if (ray.type == Ray::OCCLUSION) {
				ray.flags |= Ray::DONE; // Early out for shadow rays
			} else {
				ray.max_t = inter.t;
				inter.space = parent_xforms.size() > 0 ? lerp_seq(ray.time, parent_xforms) : Transform();
			}
		}
	}
}



void Tracer::trace_diceable_surface(DiceableSurface* prim, const std::vector<Transform>& parent_xforms, Ray* rays, Ray* end)
{
#define STACK_SIZE 64

	using namespace MicroSurfaceCache;
	int split_count = 0;

	const size_t max_subdivs = intlog2(Config::max_grid_size);

//#define USE_MICROGEO_CACHE
#ifdef USE_MICROGEO_CACHE
	// UID's
	const size_t uid1 = prim->uid; // Main UID
	size_t uid2_stack[STACK_SIZE]; // Sub-UID
	uid2_stack[0] = 1;
#endif

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

#ifdef USE_MICROGEO_CACHE
		// Fetch the current primitive's microgeo cache if it exists
		std::shared_ptr<MicroSurface> micro_surface = cache.get(Key(uid1, uid2_stack[stack_i]));
		bool rediced = false;
#else
		std::shared_ptr<MicroSurface> micro_surface;
#endif

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
			if (lerp_seq(ray.time, primitive.bounds()).intersect_ray(ray, &tnear, &tfar, ray.max_t)) {
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
#ifdef USE_MICROGEO_CACHE
						rediced = true;
#endif
					}


					// Test against the ray
					if (micro_surface->intersect_ray(ray, width, &inter)) {
						inter.hit = true;

						if (ray.type == Ray::OCCLUSION) {
							ray.flags |= Ray::DONE; // Early out for shadow rays
						} else {
							ray.max_t = inter.t;
							inter.space = parent_xforms.size() > 0 ? lerp_seq(ray.time, parent_xforms) : Transform();
						}
					}
				}
				// If it's over the max subdivisions allowed, mark for deeper traversal
				else {
					ray.flags |= Ray::DEEPER_SPLIT;
				}
			}
		} // End test potints

#ifdef USE_MICROGEO_CACHE
		// If we re-diced the microsurface, store it in the cache
		if (rediced)
			cache.put(micro_surface, Key(uid1, uid2_stack[stack_i]));
#endif

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

#ifdef USE_MICROGEO_CACHE
			const auto parent_uid2 = uid2_stack[stack_i];
#endif
			for (int i = 0; i < new_count; ++i) {
				const int ii = stack_i + i;

				// Update potint iterator stack
				ray_starts[ii] = ray_starts[stack_i];
				ray_ends[ii] = ray_ends[stack_i];

				// Update primitive stack
				std::swap(primitive_stack[ii], new_prims[i]);
#ifdef USE_MICROGEO_CACHE
				// Update uid stack
				uid2_stack[ii] = (parent_uid2 << 2) | i;
#endif
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
