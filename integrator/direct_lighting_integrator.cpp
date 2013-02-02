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



void DirectLightingIntegrator::integrate()
{
	const uint_i samp_dim = 8;

	RNG rng;
	ImageSampler image_sampler(spp, image->width, image->height);

	// Sample array
	Array<float32> samps(RAYS_AT_A_TIME * samp_dim);

	// Sample pixel coordinate array
	Array<uint16> coords(RAYS_AT_A_TIME * 2);

	// Light path array
	Array<DLPath> paths(RAYS_AT_A_TIME);

	// Ray and Intersection arrays
	Array<Ray> rays(RAYS_AT_A_TIME);
	Array<Intersection> intersections(RAYS_AT_A_TIME);

	// ids corresponding to the rays
	Array<uint32> ids(RAYS_AT_A_TIME);

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
		rays.resize(ssize);


		// Generate a bunch of camera rays
		std::cout << "\tGenerating camera rays" << std::endl;
		std::cout.flush();
		for (uint32 i = 0; i < ssize; i++) {
			float32 rx = (samps[i*samp_dim] - 0.5) * (image->max_x - image->min_x);
			float32 ry = (0.5 - samps[i*samp_dim+1]) * (image->max_y - image->min_y);
			float32 dx = (image->max_x - image->min_x) / image->width;
			float32 dy = (image->max_y - image->min_y) / image->height;
			rays[i] = scene->camera->generate_ray(rx, ry, dx, dy, samps[i*samp_dim+4], samps[i*samp_dim+2], samps[i*samp_dim+3]);
			rays[i].finalize();
			ids[i] = i;
		}


		// Trace the camera rays
		std::cout << "\tTracing camera rays" << std::endl;
		std::cout.flush();
		tracer->trace(rays, &intersections);


		// Update paths
		std::cout << "\tUpdating paths" << std::endl;
		std::cout.flush();
		uint32 rsize = rays.size();
		for (uint32 i = 0; i < rsize; i++) {
			if (intersections[i].hit) {
				// Ray hit something!  Store intersection data
				paths[ids[i]].inter = intersections[i];
			} else {
				// Ray didn't hit anything, done and black background
				paths[ids[i]].done = true;
				paths[ids[i]].col = Color(0.0, 0.0, 0.0);
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

				// Sample the light source
				Vec3 ld;
				paths[i].lcol = lighty->sample(intersections[i].p, samps[i*samp_dim+6], samps[i*samp_dim+7], rays[i].time, &ld)
				                * (float32)(scene->finite_lights.size());

				// Create a shadow ray for this path
				float32 d = ld.length();
				ld.normalize();
				rays[sri].o = paths[i].inter.p + paths[i].inter.offset;
				rays[sri].d = ld;
				rays[sri].time = samps[i*samp_dim+4];
				rays[sri].is_shadow_ray = true;
				rays[sri].has_differentials = false;
				rays[sri].min_t = 0.01;
				rays[sri].max_t = d;
				rays[sri].finalize();

				ids[sri] = i;

				// Increment shadow ray index
				sri++;
			}
		}
		rays.resize(sri);


		// Trace the shadow rays
		std::cout << "\tTracing shadow rays" << std::endl;
		std::cout.flush();
		tracer->trace(rays, &intersections);


		// Calculate sample colors
		std::cout << "\tCalculating sample colors" << std::endl;
		std::cout.flush();
		rsize = rays.size();
		for (uint32 i = 0; i < rsize; i++) {
			uint32 id = ids[i];
			if (intersections[i].hit) {
				// Sample was shadowed
				paths[id].done = true;
				paths[id].col = Color(0.0, 0.0, 0.0);
			} else {
				// Sample was lit
				paths[id].inter.n.normalize();
				float lambert = dot(rays[i].d, paths[id].inter.n);
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
}

