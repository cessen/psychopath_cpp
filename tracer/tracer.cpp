#include "tracer.hpp"

#include <vector>
#include <iostream>
#include <algorithm>
#include <assert.h>
#include "array.hpp"
#include "counting_sort.hpp"

#include "ray.hpp"
#include "scene.hpp"
#include "rayinter.hpp"

#define RAY_STATE_SIZE scene->world.ray_state_size()
#define MAX_POTINT 2
#define RAY_JOB_SIZE 2048


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


// Job class for accumulating potential intersections,
// for use in accumulate_potential_intersections() below.
class Job_accumulate_potential_intersections
{
	Tracer *tracer;
	uint_i start_i, end_i;

public:
	Job_accumulate_potential_intersections() {}

	Job_accumulate_potential_intersections(Tracer *tracer_, uint_i start_i_, uint_i end_i_) {
		tracer = tracer_;
		start_i = start_i_;
		end_i = end_i_;
	}

	// Traces rays to find potential intersections
	void operator()() {
		uint_i potint_ids[MAX_POTINT];

		for (uint_i i = start_i; i < end_i; i++) {
			const uint_i pc = tracer->scene->world.get_potential_intersections(tracer->ray_inters[i]->ray, MAX_POTINT, potint_ids, &(tracer->states[i*tracer->RAY_STATE_SIZE]));
			for (uint_i j = 0; j < pc; j++) {
				tracer->potential_inters[(i*MAX_POTINT)+j].object_id = potint_ids[j];
				tracer->potential_inters[(i*MAX_POTINT)+j].ray_inter = tracer->ray_inters[i];
			}
		}
	}
};

uint_i Tracer::accumulate_potential_intersections()
{
	// Clear out potential intersection buffer
	potential_inters.resize(ray_inters.size()*MAX_POTINT);
	const uint_i spi = potential_inters.size();
	for (uint_i i = 0; i < spi; i++)
		potential_inters[i].ray_inter = NULL;

	// Accumulate potential intersections
	JobQueue<Job_accumulate_potential_intersections> jq(thread_count);
	if (spi >= (uint_i)(thread_count)) {
		// Dole out jobs
		for (uint_i i = 0; i < ray_inters.size(); i += RAY_JOB_SIZE) {
			uint_i start = i;
			uint_i end = i + RAY_JOB_SIZE;
			if (end > ray_inters.size())
				end = ray_inters.size();

			jq.push(Job_accumulate_potential_intersections(this, start, end));
		}
		jq.finish();
	} else {
		Job_accumulate_potential_intersections job(this, 0, ray_inters.size());
		job();
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
}

void Tracer::trace_potints_consumer(JobQueue<PotintJob> *job_queue)
{
	PotintJob potint_job;

	// Keep processing items in the queue as long as they keep coming
	while (job_queue->pop(&potint_job)) {
		// Convenience
		const uint_i start = potint_job.start;
		const uint_i size = potint_job.size;

		// Trace the ray inters into the buffer
		for (uint_i i = 0; i < size; i++) {
			// Copy the rayinter into the buffer
			potint_job.ray_inters[i] = *(potential_inters[start+i].ray_inter);

			// Convenience
			RayInter *ray_inter = &(potint_job.ray_inters[i]);
			const uint_i id = potential_inters[i].object_id;

			// Trace!
			if (ray_inter->ray.is_shadow_ray) {
				if (!ray_inter->hit)
					ray_inter->hit |= scene->world.get_primitive(id).intersect_ray(ray_inter->ray, NULL);
			} else {
				ray_inter->hit |= scene->world.get_primitive(id).intersect_ray(ray_inter->ray, &(ray_inter->inter));
			}
		}
	}
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

	/*
	JobQueue<PotintJob> jq(32);
	jq.close();
	Array<RayInter> ri_list;
	ri_list.resize(RAY_JOB_SIZE);



	for (uint_i i = 0; i < potential_inters.size(); i += RAY_JOB_SIZE) {
		uint_i start = i;
		uint_i end = i + RAY_JOB_SIZE;
		if (end > potential_inters.size())
			end = potential_inters.size();

		jq.open();
		jq.push(PotintJob(start, end, &(ri_list[0])) );
		jq.close();
		trace_potints_consumer(&jq);

		for (uint_i j = 0; j < ri_list.size(); j++) {
			if (ri_list[i].hit) {
				potential_inters[start+i].ray_inter->hit = true;

				if(potential_inters[start+i].ray_inter->inter.t > ri_list[i].inter.t)
					*(potential_inters[start+i].ray_inter) = ri_list[i];
			}
		}
	}*/
}



