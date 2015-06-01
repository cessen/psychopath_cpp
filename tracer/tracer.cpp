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
#include "range.hpp"
#include "low_level.hpp"

#include "bilinear.hpp"
#include "bicubic.hpp"

#include "ray.hpp"
#include "intersection.hpp"
#include "assembly.hpp"
#include "scene.hpp"
#include "bvh.hpp"
#include "bvh2.hpp"
#include "bvh4.hpp"

#include "surface_closure.hpp"
#include "closure_union.hpp"

#define SPLIT_STACK_SIZE 64

uint32_t Tracer::trace(const WorldRay* w_rays_begin, const WorldRay* w_rays_end, Intersection* intersections_begin, Intersection* intersections_end)
{
	// Get rays
	w_rays = make_range(w_rays_begin, w_rays_end);
	Global::Stats::rays_shot += w_rays.size();

	// Create initial rays
	rays.resize(w_rays.size());
	for (size_t i = 0; i < rays.size(); ++i) {
		rays[i] = w_rays[i].to_ray();
		rays[i].set_id(i);
	}

	// Get and initialize intersections
	intersections = make_range(intersections_begin, intersections_end);
	std::fill(intersections.begin(), intersections.end(), Intersection());

	// Start tracing!
	xform_stack.push_frame<Transform>(0);

#if 0
	// Split rays into groups based on primary direction before tracing
	auto split2 = std::partition(rays.begin(), rays.end(), [](Ray r) {
		return std::abs(r.d[0]) > std::abs(r.d[1]) &&
		       std::abs(r.d[0]) > std::abs(r.d[2]);
	});
	auto split1 = std::partition(rays.begin(), split2, [](Ray r) {
		return r.d[0] > 0.0f;
	});
	trace_assembly(scene->root.get(), &(*rays.begin()), &(*split1));
	trace_assembly(scene->root.get(), &(*split1), &(*split2));

	auto split4 = std::partition(split2, rays.end(), [](Ray r) {
		return std::abs(r.d[1]) > std::abs(r.d[2]);
	});
	auto split3 = std::partition(split2, split4, [](Ray r) {
		return r.d[1] > 0.0f;
	});
	trace_assembly(scene->root.get(), &(*split2), &(*split3));
	trace_assembly(scene->root.get(), &(*split3), &(*split4));

	auto split5 = std::partition(split4, rays.end(), [](Ray r) {
		return r.d[2] > 0.0f;
	});
	trace_assembly(scene->root.get(), &(*split4), &(*split5));
	trace_assembly(scene->root.get(), &(*split5), &(*rays.end()));
#else
	// Just trace all the rays together
	trace_assembly(scene->root.get(), &(*rays.begin()), &(*rays.end()));
#endif

	return w_rays.size();
}



void Tracer::trace_assembly(Assembly* assembly, Ray* rays, Ray* rays_end)
{
	BVH4StreamTraverser traverser;

	// Initialize traverser
	traverser.init_accel(assembly->object_accel);
	traverser.init_rays(rays, rays_end);

	// Trace rays one object at a time
	std::tuple<Ray*, Ray*, size_t> hits = traverser.next_object();
	while (std::get<0>(hits) != std::get<1>(hits)) {
		const auto& instance = assembly->instances[std::get<2>(hits)]; // Short-hand for the current instance

		// Propagate transforms (if necessary)
		const auto parent_xforms = xform_stack.top_frame<Transform>();
		const size_t parent_xforms_count = std::distance(parent_xforms.first, parent_xforms.second);
		if (instance.transform_count > 0) {
			const auto xbegin = &(*(assembly->xforms.begin() + instance.transform_index));
			const auto xend = xbegin + instance.transform_count;
			const auto larger_xform_count = std::max(instance.transform_count, parent_xforms_count);

			// Push merged transforms onto transform stack
			auto xforms = xform_stack.push_frame<Transform>(larger_xform_count);
			merge(xforms.first, parent_xforms.first, parent_xforms.second, xbegin, xend);

			for (auto ray = std::get<0>(hits); ray != std::get<1>(hits); ++ray) {
				w_rays[ray->id()].update_ray(ray, lerp_seq(ray->time, xforms.first, xforms.second));
			}
		}

		// Trace against the object or assembly
		if (instance.type == Instance::OBJECT) {
			Object* obj = assembly->objects[instance.data_index].get(); // Short-hand for the current object

			// Branch to different code path based on object type
			switch (obj->get_type()) {
				case Object::SURFACE:
					trace_surface(reinterpret_cast<Surface*>(obj), std::get<0>(hits), std::get<1>(hits));
					break;
				case Object::PATCH_SURFACE:
					trace_patch_surface(reinterpret_cast<PatchSurface*>(obj), std::get<0>(hits), std::get<1>(hits));
					break;
				default:
					//std::cout << "WARNING: unknown object type, skipping." << std::endl;
					break;
			}

			Global::Stats::object_ray_tests += std::distance(std::get<0>(hits), std::get<1>(hits));
		} else { /* Instance::ASSEMBLY */
			Assembly* asmb = assembly->assemblies[instance.data_index].get(); // Short-hand for the current object
			trace_assembly(asmb, std::get<0>(hits), std::get<1>(hits));
		}

		// Un-transform rays if we transformed them earlier
		if (instance.transform_count > 0) {
			if (parent_xforms_count > 0) {
				for (auto ray = std::get<0>(hits); ray != std::get<1>(hits); ++ray) {
					w_rays[ray->id()].update_ray(ray, lerp_seq(ray->time, parent_xforms.first, parent_xforms.second));
				}
			} else {
				for (auto ray = std::get<0>(hits); ray != std::get<1>(hits); ++ray) {
					w_rays[ray->id()].update_ray(ray);
				}
			}

			// Pop top off of xform stack
			xform_stack.pop_frame();
		}


		// Get next object to test against
		hits = traverser.next_object();
	}
}



void Tracer::trace_surface(Surface* surface, Ray* rays, Ray* end)
{
	// Get parent transforms
	const auto parent_xforms = xform_stack.top_frame<Transform>();
	const size_t parent_xforms_count = std::distance(parent_xforms.first, parent_xforms.second);

	// Trace!
	for (auto ritr = rays; ritr != end; ++ritr) {
		Ray& ray = *ritr;  // Shorthand reference to the ray
		Intersection& inter = intersections[ritr->id()]; // Shorthand reference to ray's intersection

		// Test against the ray
		if (surface->intersect_ray(ray, &inter)) {
			inter.hit = true;

			if (ray.is_occlusion()) {
				ray.set_done_true(); // Early out for shadow rays
			} else {
				ray.max_t = inter.t;
				inter.space = parent_xforms_count > 0 ? lerp_seq(ray.time, parent_xforms.first, parent_xforms.second) : Transform();
				inter.surface_closure.init(GTRClosure(Color(0.9, 0.9, 0.9), 0.0f, 1.5f, 0.25f));
				//inter.surface_closure.init(LambertClosure(Color(inter.geo.u*0.9f, inter.geo.v*0.9f, 0.2f)));
				//inter.surface_closure.init(LambertClosure(Color(0.9f, 0.9f, 0.9f)));
			}
		}
	}
}

template <typename PATCH>
void intersect_rays_with_patch(const PATCH &patch, const Range<const Transform*> parent_xforms, Ray* ray_begin, Ray* ray_end, Intersection *intersections, Stack* data_stack)
{
	const size_t tsc = patch.verts.size(); // Time sample count
	int stack_i = 0;
	std::pair<Ray*, Ray*> ray_stack[SPLIT_STACK_SIZE];
	BBox* bboxes = data_stack->push_frame<BBox>(tsc).first;
	Stack &patch_stack = *data_stack;
	std::tuple<float, float, float, float> uv_stack[SPLIT_STACK_SIZE]; // (min_u, max_u, min_v, max_v)

	// Initialize stacks
	// TODO: take into account ray time
	ray_stack[0] = std::make_pair(ray_begin, ray_end);
	auto tmp = patch_stack.push_frame<typename PATCH::store_type>(tsc).first;
	for (unsigned int i = 0; i < tsc; ++i) {
		tmp[i] = patch.verts[i];
	}
	uv_stack[0] = std::tuple<float, float, float, float>(0.0f, 1.0f, 0.0f, 1.0f);

	// Iterate down to find an intersection
	while (stack_i >= 0) {
		auto cur_patches = patch_stack.top_frame<typename PATCH::store_type>().first;

		// Calculate bounding boxes and max_dim
		bboxes[0] = PATCH::bound(cur_patches[0]);
		float max_dim = longest_axis(bboxes[0].max - bboxes[0].min);
		for (unsigned int i = 1; i < tsc; ++i) {
			bboxes[i] = PATCH::bound(cur_patches[i]);
			max_dim = std::max(max_dim, longest_axis(bboxes[i].max - bboxes[i].min));
		}

		// TEST RAYS AGAINST BBOX
		ray_stack[stack_i].first = mutable_partition(ray_stack[stack_i].first, ray_stack[stack_i].second, [&](Ray& ray) {
			if (ray.is_done()) {
				return true;
			}

			// Time interpolation values, which may be used twice in the case
			// that there is more than one time sample, so store them outside
			// of the if statement below.
			float t_time;
			size_t t_index;
			float t_nalpha;

			// Ray test
			float hitt0, hitt1;
			bool hit;
			if (tsc == 1) {
				// If we only have one time sample, we can skip the bbox interpolation
				hit = bboxes[0].intersect_ray(ray, &hitt0, &hitt1, ray.max_t);
			} else {
				// If we have more than one time sample, we need to interpolate the bbox
				// before testing.
				t_time = ray.time * (tsc - 1);
				t_index = t_time;
				t_nalpha = t_time - t_index;
				hit = lerp(t_nalpha, bboxes[t_index], bboxes[t_index+1]).intersect_ray(ray, &hitt0, &hitt1, ray.max_t);
			}

			if (hit) {
				const float width = std::max(ray.min_width(hitt0, hitt1) * Config::dice_rate, Config::min_upoly_size);
				// LEAF, so we don't have to go deeper, regardless of whether
				// we hit it or not.
				if (max_dim <= width || stack_i == (SPLIT_STACK_SIZE-1)) {
					const float tt = (hitt0 + hitt1) * 0.5f;
					if (tt > 0.0f && tt < ray.max_t) {
						auto &inter = intersections[ray.id()];
						inter.hit = true;
						if (ray.is_occlusion()) {
							ray.set_done_true();
						} else {
							// Get the time-interpolated patch, for calculating
							// surface derivatives and normals below
							typename PATCH::store_type ipatch;
							if (tsc == 1) {
								// If we only have one time sample, we can skip the interpolation
								ipatch = patch.verts[0];
							} else {
								// If we have more than one time sample, we need to interpolate the patch
								ipatch = PATCH::interpolate_patch(t_nalpha, patch.verts[t_index], patch.verts[t_index+1]);
							}


							// Fill in intersection and ray info
							ray.max_t = tt;

							const float u = (std::get<0>(uv_stack[stack_i]) + std::get<1>(uv_stack[stack_i])) * 0.5f;
							const float v = (std::get<2>(uv_stack[stack_i]) + std::get<3>(uv_stack[stack_i])) * 0.5f;
							const float offset = max_dim * 1.74f;

							inter.t = tt;

							inter.space = parent_xforms.size() > 0 ? lerp_seq(ray.time, parent_xforms) : Transform();

							inter.geo.p = ray.o + (ray.d * tt);
							inter.geo.u = u;
							inter.geo.v = v;

							// Surface normal and differential geometry
							std::tie(inter.geo.n, inter.geo.dpdu, inter.geo.dpdv, inter.geo.dndu, inter.geo.dndv) = PATCH::differential_geometry(ipatch, u, v);

							// Did te ray hit from the back-side of the surface?
							inter.backfacing = dot(inter.geo.n, ray.d.normalized()) > 0.0f;

							inter.offset = inter.geo.n * offset;

							//inter.surface_closure.init(GTRClosure(Color(0.9, 0.9, 0.9), 0.02f, 1.2f, 0.25f));
							//inter.surface_closure.init(GTRClosure(Color(0.9, 0.9, 0.9), 0.0f, 1.5f, 0.25f));
							inter.surface_closure.init(LambertClosure(Color(0.9f, 0.9f, 0.9f)));
							//inter.surface_closure.init(EmitClosure(Color((1.0+inter.geo.dpdv.normalize().x)*0.5, (1.0+inter.geo.dpdv.normalize().y)*0.5, (1.0+inter.geo.dpdv.normalize().z)*0.5)));
						}
					}

					return true;
				}
				// INNER, so we have to go deeper
				else {
					return false;
				}
			} else {
				// Didn't hit it, so we don't have to go deeper
				return true;
			}
		});
		// END TEST RAYS AGAINST BBOX

		// Split patch for further traversal if necessary
		if (ray_stack[stack_i].first != ray_stack[stack_i].second) {
			auto uv = uv_stack[stack_i];
			auto next_patches = patch_stack.push_frame<typename PATCH::store_type>(tsc).first;

			const float ulen = PATCH::ulen(cur_patches[0]);
			const float vlen = PATCH::vlen(cur_patches[0]);

			// Split U
			if (ulen > vlen) {
				for (unsigned int i = 0; i < tsc; ++i) {
					PATCH::split_u(cur_patches[i], &(cur_patches[i]), &(next_patches[i]));
				}

				// Fill in uv's
				std::get<0>(uv_stack[stack_i]) = std::get<0>(uv);
				std::get<1>(uv_stack[stack_i]) = (std::get<0>(uv) + std::get<1>(uv)) * 0.5;
				std::get<2>(uv_stack[stack_i]) = std::get<2>(uv);
				std::get<3>(uv_stack[stack_i]) = std::get<3>(uv);

				std::get<0>(uv_stack[stack_i+1]) = (std::get<0>(uv) + std::get<1>(uv)) * 0.5;
				std::get<1>(uv_stack[stack_i+1]) = std::get<1>(uv);
				std::get<2>(uv_stack[stack_i+1]) = std::get<2>(uv);
				std::get<3>(uv_stack[stack_i+1]) = std::get<3>(uv);

			}
			// Split V
			else {
				for (unsigned int i = 0; i < tsc; ++i) {
					PATCH::split_v(cur_patches[i], &(cur_patches[i]), &(next_patches[i]));
				}

				// Fill in uv's
				std::get<0>(uv_stack[stack_i]) = std::get<0>(uv);
				std::get<1>(uv_stack[stack_i]) = std::get<1>(uv);
				std::get<2>(uv_stack[stack_i]) = std::get<2>(uv);
				std::get<3>(uv_stack[stack_i]) = (std::get<2>(uv) + std::get<3>(uv)) * 0.5;

				std::get<0>(uv_stack[stack_i+1]) = std::get<0>(uv);
				std::get<1>(uv_stack[stack_i+1]) = std::get<1>(uv);
				std::get<2>(uv_stack[stack_i+1]) = (std::get<2>(uv) + std::get<3>(uv)) * 0.5;
				std::get<3>(uv_stack[stack_i+1]) = std::get<3>(uv);
			}

			ray_stack[stack_i + 1] = ray_stack[stack_i];

			++stack_i;
		} else {
			--stack_i;
			patch_stack.pop_frame();
		}
	}

	patch_stack.pop_frame(); // Pop BBoxes
}


void Tracer::trace_patch_surface(PatchSurface* surface, Ray* rays, Ray* end)
{
	// Get parent transforms
	const auto parent_xforms = Range<const Transform*>(xform_stack.top_frame<Transform>());

	// Trace!
	if (auto patch = dynamic_cast<Bilinear*>(surface)) {
		intersect_rays_with_patch<Bilinear>(*patch, parent_xforms, rays, end, &(intersections[0]), &data_stack);
	} else if (auto patch = dynamic_cast<Bicubic*>(surface)) {
		intersect_rays_with_patch<Bicubic>(*patch, parent_xforms, rays, end, &(intersections[0]), &data_stack);
	}
}
