/*
 * This file and tracer.cpp define a Tracer class, which manages the tracing
 * of rays in a scene.
 */
#ifndef TRACER_HPP
#define TRACER_HPP

#include <vector>

#include "numtype.h"
#include "array.hpp"
#include "slice.hpp"



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
class Tracer
{
public:
	Scene *scene;
	BVHStreamTraverser traverser;
	Slice<const WorldRay> w_rays; // Rays to trace
	Slice<Intersection> intersections; // Resulting intersections

	Tracer(Scene *scene_): scene {scene_} {
		traverser.init_accel(scene->world);
	}


	/**
	 * Traces the provided rays, filling in the corresponding intersections.
	 *
	 * @param [in] rays_ The rays to be traced.
	 * @param [out] intersections_ The resulting intersections.
	 */
	uint32_t trace(const Slice<WorldRay> w_rays_, Slice<Intersection> intersections_);


	void trace_diceable_surface(DiceableSurfacePrimitive* prim, Ray* rays, Ray* end);
};

#endif // TRACER_HPP

