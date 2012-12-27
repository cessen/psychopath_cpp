#include "tracer.hpp"

#include <vector>
#include <iostream>
#include <algorithm>
#include <assert.h>
#include <boost/thread.hpp>
#include "array.hpp"
#include "counting_sort.hpp"

#include "ray.hpp"
#include "scene.hpp"
#include "rayinter.hpp"

#define RAY_STATE_SIZE scene->world.ray_state_size()
#define MAX_POTINT 2

uint32 Tracer::queue_rays(const Array<RayInter *> &ray_inters_)
{
	const uint32 size = ray_inters.size() + ray_inters_.size();
	ray_inters.reserve(size);

	uint32 s = ray_inters_.size();
	for (uint32 i = 0; i < s; i++) {
		ray_inters.push_back(ray_inters_[i]);
	}
	return size;
}

uint32 Tracer::trace_rays()
{
	uint32 s = ray_inters.size();
	std::cout << "\tTracing " << s << " rays" << std::endl;

	// Allocate and clear out ray states
	states.resize(s*RAY_STATE_SIZE);
	for (uint_i i = 0; i < s*RAY_STATE_SIZE; i++)
		states[i] = 0;

	// Allocate potential intersection buffer
	potential_inters.resize(s*MAX_POTINT);

	// Trace potential intersections
	while (accumulate_potential_intersections()) {
		sort_potential_intersections();
		trace_potential_intersections();
	}

	ray_inters.clear();
	return s;
}



uint_i Tracer::accumulate_potential_intersections()
{
	// Clear out potential intersection buffer
	potential_inters.resize(ray_inters.size()*MAX_POTINT);
	const uint_i spi = potential_inters.size();
	for (uint_i i = 0; i < spi; i++)
		potential_inters[i].ray_inter = NULL;

	// Accumulate potential intersections
	JobQueue<IndexRange> jq(32);
	boost::thread acc_consumers[thread_count];
	if (spi >= (uint_i)(thread_count)) {
		// Start consumer threads
		for (uint_i i=0; i < (uint_i)thread_count; i++) {
			acc_consumers[i] = boost::thread(&Tracer::accumulation_consumer, this, &jq);
		}

#define RAY_JOB_SIZE 2048
		// Dole out jobs
		for (uint_i i = 0; i < ray_inters.size(); i += RAY_JOB_SIZE) {
			uint_i start = i;
			uint_i end = i + RAY_JOB_SIZE;
			if (end > ray_inters.size())
				end = ray_inters.size();

			jq.push(IndexRange(start, end));
		}
		jq.close();

		// Join threads
		for (int i=0; i < thread_count; i++) {
			acc_consumers[i].join();
		}
	} else {
		jq.push(IndexRange(0, ray_inters.size()));
		jq.close();
		accumulation_consumer(&jq);
	}

	// Compact the potential intersections
	uint_i pii = 0;
	uint_i last = 0;
	uint_i i = 0;
	while (i < potential_inters.size()) {
		while (potential_inters[last].ray_inter != NULL && last < potential_inters.size())
			last++;

		if (potential_inters[i].ray_inter != NULL) {
			pii++;
			if (i > last) {
				potential_inters[last] = potential_inters[i];
				potential_inters[i].ray_inter = NULL;
			}
		}

		i++;
	}
	potential_inters.resize(pii);

	// Return the total number of potential intersections accumulated
	return pii;
}

void Tracer::accumulation_consumer(JobQueue<IndexRange> *job_queue)
{
	uint_i potint_ids[MAX_POTINT];
	IndexRange ir;

	// Keep processing items in the queue as long as they keep coming
	while (job_queue->pop(&ir)) {
		for (uint_i i = ir.start; i < ir.end; i++) {
			const uint_i pc = scene->world.get_potential_intersections(ray_inters[i]->ray, MAX_POTINT, potint_ids, &(states[i*RAY_STATE_SIZE]));
			for (uint_i j = 0; j < pc; j++) {
				potential_inters[(i*MAX_POTINT)+j].object_id = potint_ids[j];
				potential_inters[(i*MAX_POTINT)+j].ray_inter = ray_inters[i];
			}
		}
	}
}

void Tracer::sort_potential_intersections()
{
	const uint_i max_items = scene->world.max_primitive_id()+1;

	for (uint_i i = 0; i < max_items; i++) {
		item_counts[i] = 0;
	}

	// Count the items
	for (uint_i i = 0; i < potential_inters.size(); i++) {
		item_counts[potential_inters[i].object_id]++;
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
	while (i < potential_inters.size()) {
		const uint_i index = potential_inters[i].object_id;
		const uint_i next_place = item_start_indices[index] + item_fill_counts[index];

		if (i >= item_start_indices[index] && i < next_place) {
			i++;
		} else {
			std::swap(potential_inters[i], potential_inters[next_place]);
			item_fill_counts[index]++;
		}
		traversal++;
	}

	/*for (uint_i i = 0; i < potential_inters.size(); i++) {
		std::cout << potential_inters[i].object_id << ", ";
	}
	std::cout << "END" << std::endl;*/
}

void Tracer::trace_potential_intersections()
{
	const uint64 spi = potential_inters.size();

	for (uint32 i = 0; i < spi; i++) {
		RayInter *ray_inter = potential_inters[i].ray_inter;
		uint64 id = potential_inters[i].object_id;

		if (ray_inter->ray.is_shadow_ray) {
			if (!ray_inter->hit)
				ray_inter->hit |= scene->world.get_primitive(id).intersect_ray(ray_inter->ray, NULL);
		} else {
			ray_inter->hit |= scene->world.get_primitive(id).intersect_ray(ray_inter->ray, &(ray_inter->inter));
		}
	}
}



