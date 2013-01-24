#include "direct_lighting_integrator.hpp"

#include <iostream>
#include <assert.h>
#include "array.hpp"
#include "image_sampler.hpp"
#include "intersection.hpp"
#include "tracer.hpp"
#include "config.hpp"

#include "light.hpp"

#define RAYS_AT_A_TIME 1000000


void resize_rayinters(Array<RayInter *> &rayinters, uint32 size)
{
	if (size > rayinters.size()) {
		// Too small: enlarge
		rayinters.reserve(size);
		uint32 diff = size - rayinters.size();
		for (uint32 i = 0; i < diff; i++) {
			rayinters.push_back(new RayInter);
		}
	} else if (size < rayinters.size()) {
		// Too large: shrink
		uint32 s = rayinters.size();
		for (uint32 i = size; i < s; i++)
			delete rayinters[i];
		rayinters.resize(size);
	}
}


void DirectLightingIntegrator::integrate()
{
	const uint_i samp_dim = 8;

	RNG rng;
	ImageSampler image_sampler(spp, image->width, image->height);

	// Sample array
	Array<float32> samps;
	samps.resize(RAYS_AT_A_TIME * samp_dim);

	// Sample pixel coordinate array
	Array<uint16> coords;
	coords.resize(RAYS_AT_A_TIME * 2);

	// Light path array
	Array<DLPath> paths;
	paths.resize(RAYS_AT_A_TIME);

	// Ray array
	Array<RayInter *> rayinters;
	rayinters.reserve(RAYS_AT_A_TIME);

	bool last = false;
	while (true) {
		// Generate a bunch of samples
		std::cout << "\t--------\n\tGenerating samples" << std::endl;
		std::cout.flush();
		for (int i = 0; i < RAYS_AT_A_TIME; i++) {
			if (!image_sampler.get_next_sample(samp_dim, &(samps[i*samp_dim]), &(coords[i*2]))) {
				samps.resize(i*samp_dim);
				paths.resize(i);
				last = true;
				break;
			} else {
				paths[i].done = false;
			}
		}
		uint32 ssize = samps.size() / samp_dim;


		// Size the ray buffer appropriately
		resize_rayinters(rayinters, ssize);


		// Generate a bunch of camera rays
		std::cout << "\tGenerating camera rays" << std::endl;
		std::cout.flush();
		for (uint32 i = 0; i < ssize; i++) {
			float32 rx = (samps[i*samp_dim] - 0.5) * (image->max_x - image->min_x);
			float32 ry = (0.5 - samps[i*samp_dim+1]) * (image->max_y - image->min_y);
			float32 dx = (image->max_x - image->min_x) / image->width;
			float32 dy = (image->max_y - image->min_y) / image->height;
			rayinters[i]->ray = scene->camera->generate_ray(rx, ry, dx, dy, samps[i*samp_dim+4], samps[i*samp_dim+2], samps[i*samp_dim+3]);
			rayinters[i]->ray.finalize();
			rayinters[i]->hit = false;
			rayinters[i]->id = i;
		}


		// Trace the camera rays
		std::cout << "\tTracing camera rays" << std::endl;
		std::cout.flush();
		tracer->queue_rays(rayinters);
		tracer->trace_rays();


		// Update paths
		std::cout << "\tUpdating paths" << std::endl;
		std::cout.flush();
		uint32 rsize = rayinters.size();
		for (uint32 i = 0; i < rsize; i++) {
			if (rayinters[i]->hit) {
				// Ray hit something!  Store intersection data
				paths[rayinters[i]->id].inter = rayinters[i]->inter;
			} else {
				// Ray didn't hit anything, done and black background
				paths[rayinters[i]->id].done = true;
				paths[rayinters[i]->id].col = Color(0.0, 0.0, 0.0);
			}
		}


		// Generate a bunch of shadow rays
		std::cout << "\tGenerating shadow rays" << std::endl;
		std::cout.flush();
		uint32 sri = 0; // Shadow ray index
		for (uint32 i = 0; i < ssize; i++) {
			if (!paths[i].done) {
				// Select a light and store the normalization factor for it's output
				Light *lighty = scene->finite_lights[(uint32)(samps[i*samp_dim+5] * scene->finite_lights.size()) % scene->finite_lights.size()];
				//Light *lighty = scene->finite_lights[rng.next_uint() % scene->finite_lights.size()];

				// Sample the light source
				Vec3 ld;
				paths[i].lcol = lighty->sample(rayinters[i]->inter.p, samps[i*samp_dim+6], samps[i*samp_dim+7], rayinters[i]->ray.time, &ld)
				                * (float32)(scene->finite_lights.size());
				//paths[i].lcol = lighty->sample(rayinters[i]->inter.p, rng.next_float(), rng.next_float(), rayinters[i]->ray.time, &ld)
				//                * (float32)(scene->finite_lights.size());

				// Create a shadow ray for this path
				float d = ld.length();
				ld.normalize();
				rayinters[sri]->ray.o = paths[i].inter.p;
				rayinters[sri]->ray.d = ld;
				rayinters[sri]->ray.time = samps[i*samp_dim+4];
				rayinters[sri]->ray.is_shadow_ray = true;
				rayinters[sri]->ray.has_differentials = false;
				rayinters[sri]->ray.min_t = 0.01;
				rayinters[sri]->ray.max_t = d;
				rayinters[sri]->ray.finalize();
				rayinters[sri]->hit = false;
				rayinters[sri]->id = i;

				// Increment shadow ray index
				sri++;
			}
		}
		resize_rayinters(rayinters, sri);


		// Trace the shadow rays
		std::cout << "\tTracing shadow rays" << std::endl;
		std::cout.flush();
		tracer->queue_rays(rayinters);
		tracer->trace_rays();


		// Calculate sample colors
		std::cout << "\tCalculating sample colors" << std::endl;
		std::cout.flush();
		rsize = rayinters.size();
		for (uint32 i = 0; i < rsize; i++) {
			uint32 id = rayinters[i]->id;
			if (rayinters[i]->hit) {
				// Sample was shadowed
				paths[id].done = true;
				paths[id].col = Color(0.0, 0.0, 0.0);
			} else {
				// Sample was lit
				paths[id].inter.n.normalize();
				float lambert = dot(rayinters[i]->ray.d, paths[id].inter.n);
				if (lambert < 0.0) lambert = 0.0;

				paths[id].col = paths[id].lcol * lambert;
			}
		}


		// Accumulate the samples
		std::cout << "\tAccumulating samples" << std::endl;
		std::cout.flush();
		for (uint_i i = 0; i < ssize; i++) {
			image->add_sample(paths[i].col, coords[i*2], coords[i*2+1]);
		}

		// Print percentage complete
		static int32 last_perc = -1;
		int32 perc = image_sampler.percentage() * 100;
		if (perc > last_perc) {
			std::cout << perc << "%" << std::endl;
			last_perc = perc;
		}

		if (callback)
			callback();

		if (last)
			break;
	}

	for (uint32 i = 0; i < rayinters.size(); i++) {
		delete rayinters[i];
	}
}

