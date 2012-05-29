/*
 * This file and tracer.cpp define a Tracer class, which manages the tracing
 * of rays in a scene.
 */
#ifndef TRACER_H
#define TRACER_H

#include "numtype.h"

/*
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
 * queue_ray(), and then trace them all by calling trace_rays().  The
 * resulting intersection data is stored in the rays' data structures directly.
 * Wash, rinse, repeat.
 *
 * TODO: add a more efficient method for queuing rays, e.g. en-mass instead of
 *       one-at-a-time.
 */
class Tracer
{
public:
	// Adds a single ray to the ray queue for tracing.
	// Returns the number of rays currently queued for tracing (includes
	// the ray queued with the call, so e.g. queuing the first ray will
	// return 1).
	uint32 queue_rays(Ray *ray);

	// Traces all queued rays, and returns the number of rays traced.
	uint32 trace_rays();
}

#endif // TRACER_H

