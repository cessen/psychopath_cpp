#include "vis_integrator.hpp"

#include <iostream>
#include <assert.h>
#include <vector>

#include "image_sampler.hpp"
#include "intersection.hpp"
#include "tracer.hpp"
#include "config.hpp"

#include "light.hpp"

#define RAYS_AT_A_TIME 1000000


void VisIntegrator::integrate()
{
	const size_t samp_dim = 8;

	RNG rng;
	ImageSampler image_sampler(spp, image->width, image->height);

	// Sample array
	std::vector<float> samps;
	samps.resize(RAYS_AT_A_TIME * samp_dim);

	// Sample pixel coordinate array
	std::vector<uint16_t> coords;
	coords.resize(RAYS_AT_A_TIME * 2);

	// Light path array
	std::vector<VisPath> paths;
	paths.resize(RAYS_AT_A_TIME);

	// Ray and Intersection arrays
	std::vector<Ray> rays(RAYS_AT_A_TIME);
	std::vector<Intersection> intersections(RAYS_AT_A_TIME);

	// ids corresponding to the rays
	std::vector<uint32_t> ids(RAYS_AT_A_TIME);

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
		uint32_t ssize = samps.size() / samp_dim;


		// Size the ray buffer appropriately
		rays.resize(ssize);

		// Generate a bunch of camera rays
		std::cout << "\tGenerating camera rays" << std::endl;
		std::cout.flush();
		for (uint32_t i = 0; i < ssize; i++) {
			float rx = (samps[i*samp_dim] - 0.5) * (image->max_x - image->min_x);
			float ry = (0.5 - samps[i*samp_dim+1]) * (image->max_y - image->min_y);
			float dx = (image->max_x - image->min_x) / image->width;
			float dy = (image->max_y - image->min_y) / image->height;
			rays[i] = scene->camera->generate_ray(rx, ry, dx, dy, samps[i*samp_dim+4], samps[i*samp_dim+2], samps[i*samp_dim+3]);
			rays[i].finalize();
			ids[i] = i;
		}


		// Trace the camera rays
		std::cout << "\tTracing camera rays" << std::endl;
		std::cout.flush();
		tracer->trace(&(*rays.begin()), &(*rays.end()), &(*intersections.begin()), &(*intersections.end()));


		// Update paths
		std::cout << "\tUpdating paths" << std::endl;
		std::cout.flush();
		uint32_t rsize = rays.size();
		for (uint32_t i = 0; i < rsize; i++) {
			if (intersections[i].hit) {
				// Ray hit something!  Store intersection data
				paths[ids[i]].done = true;
				paths[ids[i]].col = intersections[i].col;
			} else {
				// Ray didn't hit anything, done and black background
				paths[ids[i]].done = true;
				paths[ids[i]].col = Color(0.0, 0.0, 0.0);
			}
		}


		// Print percentage complete
		static int32_t last_perc = -1;
		int32_t perc = image_sampler.percentage() * 100;
		if (perc > last_perc) {
			std::cout << perc << "%" << std::endl;
			last_perc = perc;
		}

		if (!Config::no_output) {
			// Accumulate the samples
			std::cout << "\tAccumulating samples" << std::endl;
			std::cout.flush();
			for (size_t i = 0; i < ssize; i++) {
				image->add_sample(paths[i].col, coords[i*2], coords[i*2+1]);
			}

			if (callback)
				callback();
		}

		if (last)
			break;
	}
}

