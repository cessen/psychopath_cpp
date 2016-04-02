#include "patch_utils.hpp"

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



uint32_t Tracer::trace(const WorldRay* w_rays_begin, const WorldRay* w_rays_end, Intersection* intersections_begin, Intersection* intersections_end) {
	// Clear ID
	element_id.clear();

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

	// Clear and initialize stacks
	surface_shader_stack.clear();
	surface_shader_stack.emplace_back(nullptr);
	xform_stack.clear();
	xform_stack.push_frame<Transform>(0);
	data_stack.clear();

	// Start tracing!

#if 1
	// Split rays into groups based on direction signs before tracing
	auto xsplit_4 = std::partition(rays.begin(), rays.end(), [](Ray r) {
		return (r.d[0] > 0.0f);
	});

	auto ysplit_2 = std::partition(rays.begin(), xsplit_4, [](Ray r) {
		return (r.d[1] > 0.0f);
	});
	auto ysplit_6 = std::partition(xsplit_4, rays.end(), [](Ray r) {
		return (r.d[1] > 0.0f);
	});

	auto zsplit_1 = std::partition(rays.begin(), ysplit_2, [](Ray r) {
		return (r.d[2] > 0.0f);
	});
	auto zsplit_3 = std::partition(ysplit_2, xsplit_4, [](Ray r) {
		return (r.d[2] > 0.0f);
	});
	auto zsplit_5 = std::partition(xsplit_4, ysplit_6, [](Ray r) {
		return (r.d[2] > 0.0f);
	});
	auto zsplit_7 = std::partition(ysplit_6, rays.end(), [](Ray r) {
		return (r.d[2] > 0.0f);
	});

	// +X +Y +Z
	trace_assembly(scene->root.get(), &(*rays.begin()), &(*zsplit_1));

	// +X +Y -Z
	trace_assembly(scene->root.get(), &(*zsplit_1), &(*ysplit_2));

	// +X -Y +Z
	trace_assembly(scene->root.get(), &(*ysplit_2), &(*zsplit_3));

	// +X -Y -Z
	trace_assembly(scene->root.get(), &(*zsplit_3), &(*xsplit_4));

	// -X +Y +Z
	trace_assembly(scene->root.get(), &(*xsplit_4), &(*zsplit_5));

	// -X +Y -Z
	trace_assembly(scene->root.get(), &(*zsplit_5), &(*ysplit_6));

	// -X -Y +Z
	trace_assembly(scene->root.get(), &(*ysplit_6), &(*zsplit_7));

	// -X -Y -Z
	trace_assembly(scene->root.get(), &(*zsplit_7), &(*rays.end()));
#else
	// Just trace all the rays together
	trace_assembly(scene->root.get(), &(*rays.begin()), &(*rays.end()));
#endif

	return w_rays.size();
}



void Tracer::trace_assembly(Assembly* assembly, Ray* rays, Ray* rays_end) {
	BVH4StreamTraverser traverser;

	// Initialize traverser
	traverser.init_accel(assembly->object_accel);
	traverser.init_rays(rays, rays_end);

	// Trace rays one object at a time
	std::tuple<Ray*, Ray*, size_t> hits = traverser.next_object();
	while (std::get<0>(hits) != std::get<1>(hits)) {
		const auto& instance = assembly->instances[std::get<2>(hits)]; // Short-hand for the current instance

		// Push the current instance index onto the element id
		const auto element_id_bits = assembly->element_id_bits();
		element_id.push_back(std::get<2>(hits), element_id_bits);

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

		// Check for shader on the instance, and push to shader stack if it
		// has one.
		if (instance.surface_shader != nullptr) {
			surface_shader_stack.emplace_back(instance.surface_shader);
		}

		// Trace against the object or assembly
		if (instance.type == Instance::OBJECT) {
			Object* obj = assembly->objects[instance.data_index].get(); // Short-hand for the current object
			// Branch to different code path based on object type
			switch (obj->get_type()) {
				case Object::SURFACE:
					trace_surface(reinterpret_cast<Surface*>(obj), std::get<0>(hits), std::get<1>(hits));
					break;
				case Object::COMPLEX_SURFACE:
					trace_complex_surface(reinterpret_cast<ComplexSurface*>(obj), std::get<0>(hits), std::get<1>(hits));
					break;
				case Object::PATCH_SURFACE:
					trace_patch_surface(reinterpret_cast<PatchSurface*>(obj), std::get<0>(hits), std::get<1>(hits));
					break;
				case Object::LIGHT:
					trace_lightsource(reinterpret_cast<Light*>(obj), std::get<0>(hits), std::get<1>(hits));
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

		// Pop shader stack if we pushed onto it earlier
		if (instance.surface_shader != nullptr) {
			surface_shader_stack.pop_back();
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

		// Pop the index of this instance off the element id
		element_id.pop_back(element_id_bits);

		// Get next object to test against
		hits = traverser.next_object();
	}
}



void Tracer::trace_surface(Surface* surface, Ray* rays, Ray* end) {
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
			inter.id = element_id;

			if (ray.is_occlusion()) {
				ray.set_done_true(); // Early out for shadow rays
			} else {
				ray.max_t = inter.t;
				inter.space = parent_xforms_count > 0 ? lerp_seq(ray.time, parent_xforms.first, parent_xforms.second) : Transform();

				// Do shading
				auto shader = surface_shader_stack.back();
				if (shader != nullptr) {
					shader->shade(&inter);
				} else {
					inter.surface_closure.init(EmitClosure(Color(1.0, 0.0, 1.0)));
				}
			}
		}
	}
}




void Tracer::trace_complex_surface(ComplexSurface* surface, Ray* rays, Ray* end) {
	// Get parent transforms
	const auto parent_xforms = Range<const Transform*>(xform_stack.top_frame<Transform>());

	// Trace!
	surface->intersect_rays(rays, end,
	                        &(intersections[0]),
	                        parent_xforms,
	                        &data_stack,
	                        surface_shader_stack.back(),
	                        element_id
	                       );
}




void Tracer::trace_patch_surface(PatchSurface* surface, Ray* rays, Ray* end) {
	// Get parent transforms
	const auto parent_xforms = Range<const Transform*>(xform_stack.top_frame<Transform>());

	// Trace!
	if (auto patch = dynamic_cast<Bilinear*>(surface)) {
		intersect_rays_with_patch<Bilinear>(*patch, parent_xforms, rays, end, &(intersections[0]), &data_stack, surface_shader_stack.back(), element_id);
	} else if (auto patch = dynamic_cast<Bicubic*>(surface)) {
		intersect_rays_with_patch<Bicubic>(*patch, parent_xforms, rays, end, &(intersections[0]), &data_stack, surface_shader_stack.back(), element_id);
	}
}



void Tracer::trace_lightsource(Light* light, Ray* rays, Ray* end) {
	// Get parent transforms
	const auto parent_xforms = xform_stack.top_frame<Transform>();
	const size_t parent_xforms_count = std::distance(parent_xforms.first, parent_xforms.second);

	// Trace!
	for (auto ritr = rays; ritr != end; ++ritr) {
		Ray& ray = *ritr;  // Shorthand reference to the ray
		Intersection& inter = intersections[ritr->id()]; // Shorthand reference to ray's intersection

		// Test against the ray
		if (light->intersect_ray(ray, &inter)) {
			inter.hit = true;
			inter.id = element_id;

			if (ray.is_occlusion()) {
				ray.set_done_true(); // Early out for shadow rays
			} else {
				ray.max_t = inter.t;
				inter.space = parent_xforms_count > 0 ? lerp_seq(ray.time, parent_xforms.first, parent_xforms.second) : Transform();
			}
		}
	}
}
