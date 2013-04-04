#include "tracer.hpp"

#include <limits>
#include <vector>
#include <iostream>
#include <algorithm>
#include <functional>
#include <assert.h>

#include "array.hpp"
#include "slice.hpp"
#include "counting_sort.hpp"

#include "ray.hpp"
#include "intersection.hpp"
#include "scene.hpp"

#define RAY_STATE_SIZE scene->world.ray_state_size()
#define MAX_POTINT 8
#define RAY_JOB_SIZE (1024*4)


uint32 Tracer::trace(const Array<Ray> &rays_, Array<Intersection> *intersections_)
{
	// Get rays
	rays.init_from(rays_);

	// Get and initialize intersections
	intersections_->resize(rays.size());
	intersections.init_from(*intersections_);
	std::fill(intersections.begin(), intersections.end(), Intersection());

	// Print number of rays being traced
	std::cout << "\tTracing " << rays.size() << " rays" << std::endl;

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

		/*
		// Print number of active rays
		size_t active_rays = 0;
		for (auto r: rays_active) {
			if (r)
				active_rays++;
		}
		std::cout << "Active rays: " << active_rays << std::endl;
		*/
	}

	return rays.size();
}


// Job function for accumulating potential intersections,
// for use in accumulate_potential_intersections() below.
void job_accumulate_potential_intersections(Tracer *tracer, uint_i start_i, uint_i end_i)
{
	uint_i potint_ids[MAX_POTINT];

	for (uint_i i = start_i; i < end_i; i++) {
		if (tracer->rays_active[i]) {
			const uint_i pc = tracer->scene->world.get_potential_intersections(tracer->rays[i], tracer->intersections[i].t, MAX_POTINT, potint_ids, &(tracer->states[i*tracer->RAY_STATE_SIZE]));
			tracer->rays_active[i] = (pc > 0);

			for (uint_i j = 0; j < pc; j++) {
				tracer->potential_intersections[(i*MAX_POTINT)+j].valid = true;
				tracer->potential_intersections[(i*MAX_POTINT)+j].object_id = potint_ids[j];
				tracer->potential_intersections[(i*MAX_POTINT)+j].ray_index = i;
			}
		}
	}
}

uint_i Tracer::accumulate_potential_intersections()
{
	// Clear out potential intersection buffer
	potential_intersections.resize(rays.size()*MAX_POTINT);
	const uint_i spi = potential_intersections.size();
	for (uint_i i = 0; i < spi; i++)
		potential_intersections[i].valid = false;


	// Accumulate potential intersections
#if 1
	JobQueue<> jq(thread_count);
	for (uint_i i = 0; i < rays.size(); i += RAY_JOB_SIZE) {
		// Dole out jobs
		uint_i start = i;
		uint_i end = i + RAY_JOB_SIZE;
		if (end > rays.size())
			end = rays.size();

		jq.push(std::bind(job_accumulate_potential_intersections, this, start, end));
	}
	jq.finish();
#else
	for (uint_i i = 0; i < rays.size(); i += RAY_JOB_SIZE) {

		// Dole out jobs
		uint_i start = i;
		uint_i end = i + RAY_JOB_SIZE;
		if (end > rays.size())
			end = rays.size();


		job_accumulate_potential_intersections(this, start, end);

	}
#endif


	// Compact the potential intersections
	uint_i pii = 0;
	uint_i last = 0;
	uint_i i = 0;
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
 *
 * TODO: generate list of objects ids and where they start.
 */
void Tracer::sort_potential_intersections()
{
	//std::sort(potential_intersections.begin(), potential_intersections.end());
	//return;
	const uint_i max_items = scene->world.max_primitive_id()+1;

	for (uint_i i = 0; i < max_items; i++) {
		item_counts[i] = 0;
	}

	// Count the items
	for (uint_i i = 0; i < potential_intersections.size(); i++) {
		item_counts[potential_intersections[i].object_id]++;
	}

	// Set up start-index array
	uint_i running_count = 0;
	for (uint_i i = 0; i < max_items; i++) {
		item_start_indices[i] = running_count;
		running_count += item_counts[i];
	}

	// Set up filled-so-far-count array
	for (uint_i i = 0; i < max_items; i++) {
		item_fill_counts[i] = 0;
	}

	// Sort the list
	uint_i traversal = 0;
	uint_i i = 0;
	while (i < potential_intersections.size()) {
		const uint_i index = potential_intersections[i].object_id;
		const uint_i next_place = item_start_indices[index] + item_fill_counts[index];

		if (i >= item_start_indices[index] && i < next_place) {
			i++;
		} else {
			std::swap(potential_intersections[i], potential_intersections[next_place]);
			item_fill_counts[index]++;
		}
		traversal++;
	}
}


void job_trace_potential_intersections(Tracer *tracer, uint_i start, uint_i end)
{
	for (uint_i i = start; i < end; i++) {
		// Shorthand references
		PotentialInter &potential_intersection = tracer->potential_intersections[i];
		const Ray& ray = tracer->rays[potential_intersection.ray_index];
		Intersection& intersection = tracer->intersections[potential_intersection.ray_index];
		uint_i& id = potential_intersection.object_id;

		// Trace
		if (ray.is_shadow_ray) {
			if (!intersection.hit)
				intersection.hit |= tracer->scene->world.get_primitive(id).intersect_ray(ray, nullptr);
		} else {
			intersection.hit |= tracer->scene->world.get_primitive(id).intersect_ray(ray, &intersection);
		}
	}
}

void Tracer::trace_potential_intersections()
{
#define BLARGYFACE 10000
	JobQueue<> jq(thread_count);
	for (uint_i i = 0; i < potential_intersections.size(); i += BLARGYFACE) {
		// Dole out jobs
		uint_i start = i;
		uint_i end = i + BLARGYFACE;
		if (end > potential_intersections.size())
			end = potential_intersections.size();

		//jq.push(std::bind(job_trace_potential_intersections, this, Slice<Intersection>start, end));
		job_trace_potential_intersections(this, start, end);
	}
	jq.finish();
}



