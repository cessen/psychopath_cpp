#include "path_trace_integrator.hpp"

#include <iostream>
#include <assert.h>
#include "image_sampler.hpp"
#include "intersection.hpp"
#include "tracer.hpp"
#include "config.hpp"

#include "array.hpp"

#include "light.hpp"

#include "hilbert_curve.hpp"

#define RAYS_AT_A_TIME 500000

#define GAUSS_WIDTH 2.0 / 4
float32 gaussian(float32 x, float32 y)
{
	float32 xf = expf(-x * x / (2 * GAUSS_WIDTH * GAUSS_WIDTH));
	float32 yf = expf(-y * y / (2 * GAUSS_WIDTH * GAUSS_WIDTH));
	return xf * yf;
}

float32 mitchell_1d(float32 x, float32 C)
{
	float32 B = 1.0 - (2*C);
	x = fabsf(1.f * x);
	if (x > 2.0)
		return 0.0;
	if (x > 1.f)
		return ((-B - 6*C) * x*x*x + (6*B + 30*C) * x*x +
		        (-12*B - 48*C) * x + (8*B + 24*C)) * (1.f/6.f);
	else
		return ((12 - 9*B - 6*C) * x*x*x +
		        (-18 + 12*B + 6*C) * x*x +
		        (6 - 2*B)) * (1.f/6.f);
}

float32 mitchell_2d(float32 x, float32 y, float32 C)
{
	return mitchell_1d(x, C) * mitchell_1d(y, C);
}


float32 lambert(Vec3 v1, Vec3 v2)
{
	v1.normalize();
	v2.normalize();
	float32 f = dot(v1, v2);
	if (f < 0.0)
		f = 0.0;
	return f;
}


void PathTraceIntegrator::integrate()
{
	RNG rng(43643);
	ImageSampler image_sampler(spp, image->width, image->height, 2.0);

	// Sample array
	Array<Sample> samps;
	samps.resize(RAYS_AT_A_TIME);

	// Light path array
	Array<PTPath> paths;
	paths.resize(RAYS_AT_A_TIME);

	// Allocate the RayInter structures in contiguous storage
	RayInter *rayinters_ = new RayInter[RAYS_AT_A_TIME];

	// RayInter pointer array, points to contiguous storage above
	Array<RayInter *> rayinters;
	rayinters.reserve(RAYS_AT_A_TIME);
	for (uint32 i=0; i < RAYS_AT_A_TIME; i++) {
		rayinters.push_back(&rayinters_[i]);
	}

	bool last = false;
	while (true) {
		// Generate samples
		std::cout << "\t--------\n\tGenerating samples" << std::endl;
		std::cout.flush();
		for (int i = 0; i < RAYS_AT_A_TIME; i++) {
			paths[i].done = false;
			paths[i].col = Color(0.0);
			paths[i].fcol = Color(1.0);

			if (!image_sampler.get_next_sample(&(samps[i]), path_length*5)) {
				samps.resize(i);
				paths.resize(i);
				last = true;
				break;
			}
		}
		uint32 samp_size = samps.size();

		// Path tracing loop for the samples we have
		for (int path_n=0; path_n < path_length; path_n++) {
			std::cout << "\t-- Path segment #" << path_n << std::endl;
			std::cout.flush();

			// Size the ray buffer appropriately
			rayinters.resize(samp_size);

			int32 so = (path_n * 5) - 2; // Sample offset

			// Calculate path rays
			std::cout << "\tGenerating path rays" << std::endl;
			std::cout.flush();
			if (path_n == 0) {
				// First segment of path is camera rays
				for (uint32 i = 0; i < samp_size; i++) {
					float32 rx = (samps[i].x - 0.5) * (image->max_x - image->min_x);
					float32 ry = (0.5 - samps[i].y) * (image->max_y - image->min_y);
					float32 dx = (image->max_x - image->min_x) / image->width;
					float32 dy = (image->max_y - image->min_y) / image->height;
					rayinters[i]->ray = scene->camera->generate_ray(rx, ry, dx, dy, samps[i].t, samps[i].u, samps[i].v);
					rayinters[i]->ray.finalize();
					rayinters[i]->hit = false;
					rayinters[i]->id = i;
				}
			} else {
				// Other path segments are bounces
				uint32 pri = 0; // Path ray index
				for (uint32 i = 0; i < samp_size; i++) {
					if (!paths[i].done) {
						// Generate a random ray direction in the hemisphere
						// of the surface.
						// TODO: use BxDF distribution here
						// TODO: use proper PDF here
						Vec3 dir = cosine_sample_hemisphere(samps[i].ns[so], samps[i].ns[so+1]);
						float32 pdf = dir.z * 2;
						if (pdf < 0.001)
							pdf = 0.001;
						dir = zup_to_vec(dir, paths[i].inter.n);

						// Calculate the color filtering effect that the
						// bounce from the current intersection will create.
						// TODO: use actual shaders here.
						paths[i].fcol *= lambert(dir, paths[i].inter.n) / pdf;

						// Clear out the rayinter structure
						rayinters[pri]->hit = false;
						rayinters[pri]->id = i;

						// Create a bounce ray for this path
						rayinters[pri]->ray.o = paths[i].inter.p;
						rayinters[pri]->ray.d = dir;
						rayinters[pri]->ray.time = samps[i].t;
						rayinters[pri]->ray.is_shadow_ray = false;
						rayinters[pri]->ray.has_differentials = false;
						rayinters[pri]->ray.min_t = 0.01;
						rayinters[pri]->ray.max_t = 999999999999.0;
						rayinters[pri]->ray.finalize();

						// Increment path ray index
						pri++;
					}
				}
				rayinters.resize(pri);
			}


			// Trace the rays
			tracer->queue_rays(rayinters);
			tracer->trace_rays();


			// Update paths
			std::cout << "\tUpdating paths" << std::endl;
			std::cout.flush();
			uint32 rsize = rayinters.size();
			for (uint32 i = 0; i < rsize; i++) {
				const uint32 id = rayinters[i]->id;
				if (rayinters[i]->hit) {
					// Ray hit something!  Store intersection data
					paths[id].inter = rayinters[i]->inter;
				} else {
					// Ray didn't hit anything, done and black background
					paths[id].done = true;
					paths[id].col += Color(0.0);
				}
			}


			// Generate a bunch of shadow rays
			std::cout << "\tGenerating shadow rays" << std::endl;
			std::cout.flush();
			uint32 sri = 0; // Shadow ray index
			for (uint32 i = 0; i < paths.size(); i++) {
				if (!paths[i].done) {
					// Select a light and store the normalization factor for it's output
					Light *lighty = scene->finite_lights[(uint32)(samps[i].ns[so+2] * scene->finite_lights.size()) % scene->finite_lights.size()];
					//Light *lighty = scene->finite_lights[rng.next_uint() % scene->finite_lights.size()];

					// Sample the light source
					Vec3 ld;
					paths[i].lcol = lighty->sample(paths[i].inter.p, samps[i].ns[so+3], samps[i].ns[so+4], samps[i].t, &ld)
					                * (float32)(scene->finite_lights.size());
					//paths[i].lcol = lighty->sample(paths[i].inter.p, rng.next_float(), rng.next_float(), samps[i].t, &ld)
					//                * (float32)(scene->finite_lights.size());

					// Create a shadow ray for this path
					float d = ld.normalize();
					rayinters[sri]->ray.o = paths[i].inter.p;
					rayinters[sri]->ray.d = ld;
					rayinters[sri]->ray.time = samps[i].t;
					rayinters[sri]->ray.is_shadow_ray = true;
					rayinters[sri]->ray.has_differentials = false;
					rayinters[sri]->ray.min_t = 0.01;
					rayinters[sri]->ray.max_t = d;
					rayinters[sri]->ray.finalize();
					rayinters[sri]->hit = false;
					rayinters[sri]->id = i;
					sri++;
				}
			}
			rayinters.resize(sri);


			// Trace the shadow rays
			tracer->queue_rays(rayinters);
			tracer->trace_rays();


			// Calculate sample colors
			std::cout << "\tCalculating sample colors" << std::endl;
			std::cout.flush();
			rsize = rayinters.size();
			for (uint32 i = 0; i < rsize; i++) {
				const uint32 id = rayinters[i]->id;
				if (!rayinters[i]->hit) {
					// Sample was lit
					// TODO: use actual shaders here
					float lam = lambert(rayinters[i]->ray.d, paths[id].inter.n);
					paths[id].col += paths[id].fcol * paths[id].lcol * lam;
				}
			}
		}


		// Accumulate the samples
		std::cout << "\tAccumulating samples" << std::endl;
		std::cout.flush();
		float32 x, y;
		int32 i2;
		for (uint32 i = 0; i < samp_size; i++) {
			x = (samps[i].x * image->width) - 0.5;
			y = (samps[i].y * image->height) - 0.5;
			for (int j=-2; j <= 2; j++) {
				for (int k=-2; k <= 2; k++) {
					int a = x + j;
					int b = y + k;
					if (a < 0 || !(a < image->width) || b < 0 || !(b < image->height))
						continue;
					float32 contrib = mitchell_2d(a-x, b-y, 0.5);
					i2 = (image->width * b) + a;

					accum->pixels[i2] += contrib;
					if (contrib == 0)
						continue;

					image->pixels[i2*image->channels] += paths[i].col.spectrum[0] * contrib;
					image->pixels[(i2*image->channels)+1] += paths[i].col.spectrum[1] * contrib;
					image->pixels[(i2*image->channels)+2] += paths[i].col.spectrum[2] * contrib;
				}
			}
		}

		// Print percentage complete
		static int32 last_perc = -1;
		int32 perc = image_sampler.percentage() * 100;
		if (perc > last_perc) {
			std::cout << perc << "%" << std::endl;
			last_perc = perc;
		}

		if (last)
			break;
	}

	// Delete the RayInter structures
	delete [] rayinters_;


	// Combine all the accumulated sample
	int i;
	for (int y = 0; y < image->height; y++) {
		for (int x = 0; x < image->width; x++) {
			i = x + (y*image->width);

			image->pixels[i*image->channels] /= accum->pixels[i];
			image->pixels[i*image->channels] = std::max(image->pixels[i*image->channels], 0.0f);

			image->pixels[i*image->channels+1] /= accum->pixels[i];
			image->pixels[i*image->channels+1] = std::max(image->pixels[i*image->channels+1], 0.0f);

			image->pixels[i*image->channels+2] /= accum->pixels[i];
			image->pixels[i*image->channels+2] = std::max(image->pixels[i*image->channels+2], 0.0f);
		}
	}

	std::cout << "Splits during rendering: " << Config::split_count << std::endl;
	std::cout << "Micropolygons generated during rendering: " << Config::upoly_gen_count << std::endl;
	std::cout << "Grid cache misses during rendering: " << Config::cache_misses << std::endl;
}

