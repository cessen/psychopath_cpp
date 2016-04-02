/*
 * This file and tracer.cpp define a Tracer class, which manages the tracing
 * of rays in a scene.
 */
#ifndef TRACER_HPP
#define TRACER_HPP

#include <vector>

#include "numtype.h"
#include "range.hpp"
#include "rng.hpp"
#include "stack.hpp"

#include "instance_id.hpp"
#include "ray.hpp"
#include "intersection.hpp"
#include "potentialinter.hpp"
#include "scene.hpp"


/**
 * @brief Traces rays in a scene.
 *
 * The Tracer is responsible for doing the actual ray-tracing in a scene.
 * It does _not_ manage the specific integration algorithm, or shading.  Only
 * the tracing of rays and calculating the relevant information about ray
 * hits.
 *
 * It is specifically designed to handle tracing a large number of rays
 * (ideally > a million, as ram allows) simultaneously to gain efficiency
 * in various ways.  The rays do not need to be related to each other or
 * coherent in any way.
 *
 * It is, of course, also capable of tracing a single ray at a time or a small
 * number of rays at a time if necessary. But doing so may be far less
 * efficient depending on the scene.
 *
 * The simplest usage is to add a bunch of rays to the Tracer's queue with
 * queue_rays(), and then trace them all by calling trace_rays().  The
 * resulting intersection data is stored in the rays' data structures directly.
 * Wash, rinse, repeat.
 */
class Tracer {
public:
	Scene *scene;
	Range<const WorldRay*> w_rays; // Rays to trace
	Range<Intersection*> intersections; // Resulting intersections
	std::vector<Ray> rays;
	RNG rng;
	std::vector<const SurfaceShader*> surface_shader_stack;
	Stack xform_stack; // Stack for transforms as we traverse into transform hierarchies
	Stack data_stack; // Stack for arbitrary POD data, passed to other functions
	InstanceID element_id;
	int element_id_pos = 0;

	Tracer(): xform_stack(16*4*256*64, 256), data_stack(1024*1024*8, 256) {
		surface_shader_stack.reserve(64);
	}

	Tracer(Scene *scene_): scene {scene_}, xform_stack(16*4*256*64, 256), data_stack(1024*1024*8, 256) {
		surface_shader_stack.reserve(64);
	}

	void set_seed(uint32_t seed) {
		rng.seed(seed);
	}


	/**
	 * Traces the provided rays, filling in the corresponding intersections.
	 *
	 * @param [in] rays_ The rays to be traced.
	 * @param [out] intersections_ The resulting intersections.
	 */
	uint32_t trace(const WorldRay* w_rays_begin, const WorldRay* w_rays_end, Intersection* intersections_begin, Intersection* intersections_end);

private:
	// Various methods for tracing different object types
	void trace_assembly(Assembly* assembly, Ray* rays, Ray* rays_end);
	void trace_surface(Surface* surface, Ray* rays, Ray* end);
	void trace_complex_surface(ComplexSurface* surface, Ray* rays, Ray* end);
	void trace_patch_surface(PatchSurface* surface, Ray* rays, Ray* end);
	void trace_lightsource(Light* light, Ray* rays, Ray* end);
};

#endif // TRACER_HPP

