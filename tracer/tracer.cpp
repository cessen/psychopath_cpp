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
#define MAX_POTINT 2
#define RAY_JOB_SIZE 2048


uint32 Tracer::trace(const Array<Ray> &rays_, Array<Intersection> *intersections_)
{
	// Get rays
	rays.init_from(rays_);

	// Get and initialize intersections
	intersections_->resize(rays.size());
	intersections.init_from(*intersections_);
	for (auto &inter: intersections)
		inter = Intersection();

	// Constant for number of rays being traced
	const uint32 s = rays.size();
	std::cout << "\tTracing " << s << " rays" << std::endl;

	// Allocate and clear out ray states
	states.resize(s*RAY_STATE_SIZE);
	for (uint_i i = 0; i < s*RAY_STATE_SIZE; i++)
		states[i] = 0;

	// Allocate potential intersection buffer
	potential_intersections.resize(s*MAX_POTINT);

	// Trace potential intersections
	while (accumulate_potential_intersections()) {
		sort_potential_intersections();
		trace_potential_intersections();
	}

	return s;
}


// Job function for accumulating potential intersections,
// for use in accumulate_potential_intersections() below.
void job_accumulate_potential_intersections(Tracer *tracer, uint_i start_i, uint_i end_i)
{
	uint_i potint_ids[MAX_POTINT];

	for (uint_i i = start_i; i < end_i; i++) {
		const uint_i pc = tracer->scene->world.get_potential_intersections(tracer->rays[i], MAX_POTINT, potint_ids, &(tracer->states[i*tracer->RAY_STATE_SIZE]));
		for (uint_i j = 0; j < pc; j++) {
			tracer->potential_intersections[(i*MAX_POTINT)+j].valid = true;
			tracer->potential_intersections[(i*MAX_POTINT)+j].object_id = potint_ids[j];
			tracer->potential_intersections[(i*MAX_POTINT)+j].ray_index = i;
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
	JobQueue<std::function<void()>> jq(thread_count);
	for (uint_i i = 0; i < rays.size(); i += RAY_JOB_SIZE) {
		// Dole out jobs
		uint_i start = i;
		uint_i end = i + RAY_JOB_SIZE;
		if (end > rays.size())
			end = rays.size();

		jq.push(std::bind(job_accumulate_potential_intersections, this, start, end));
	}
	jq.finish();

	// Compact the potential intersections
	uint_i pii = 0;
	uint_i last = 0;
	uint_i i = 0;
	while (i < potential_intersections.size()) {
		while (potential_intersections[last].valid && last < potential_intersections.size())
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


void Tracer::trace_potential_intersections()
{
	for (auto potential_intersection: potential_intersections) {
		// Shorthand references for current ray, intersection, and object id
		const Ray& ray = rays[potential_intersection.ray_index];
		Intersection& intersection = intersections[potential_intersection.ray_index];
		uint_i& id = potential_intersection.object_id;

		// Trace
		if (ray.is_shadow_ray) {
			if (!intersection.hit)
				intersection.hit |= scene->world.get_primitive(id).intersect_ray(ray, nullptr);
		} else {
			intersection.hit |= scene->world.get_primitive(id).intersect_ray(ray, &intersection);
		}
	}
}



