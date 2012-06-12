#include "integrator.hpp"

#include <iostream>
#include <assert.h>
#include "image_sampler.hpp"
#include "intersection.hpp"
#include "tracer.hpp"
#include "config.hpp"

#include "light.hpp"

#define RAYS_AT_A_TIME 1000000

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


void Integrator::integrate()
{
	RNG rng(43643);
	ImageSampler image_sampler(spp, image->width, image->height, 2.0);

	std::vector<Sample> samps;
	samps.resize(RAYS_AT_A_TIME);

	std::vector<RayInter *> rayinters;
	rayinters.reserve(RAYS_AT_A_TIME);
	for (int i = 0; i < RAYS_AT_A_TIME; i++) {
		rayinters.push_back(new RayInter);
	}

	bool last = false;
	while (true) {
		// Generate a bunch of samples and corresponding rays
		std::cout << "\tGenerating rays" << std::endl;
		std::cout.flush();
		for (int i = 0; i < RAYS_AT_A_TIME; i++) {
			if (image_sampler.get_next_sample(&(samps[i]))) {
				float32 rx = (samps[i].x - 0.5) * (image->max_x - image->min_x);
				float32 ry = (0.5 - samps[i].y) * (image->max_y - image->min_y);
				float32 dx = (image->max_x - image->min_x) / image->width;
				float32 dy = (image->max_y - image->min_y) / image->height;
				rayinters[i]->ray = scene->camera->generate_ray(rx, ry, dx, dy, samps[i].t, samps[i].u, samps[i].v);
				rayinters[i]->ray.finalize();
				rayinters[i]->hit = false;
			} else {
				uint32 s = rayinters.size();
				for (uint32 i2 = i; i2 < s; i2++)
					delete rayinters[i2];
				rayinters.resize(i);
				last = true;
				break;
			}
		}


		// Trace the rays
		std::cout << "\tTracing rays" << std::endl;
		std::cout.flush();
		tracer->queue_rays(rayinters);
		tracer->trace_rays();


		// Accumulate their samples
		std::cout << "\tAccumulating samples" << std::endl;
		std::cout.flush();
		float32 x, y;
		int32 i2;
		int s = rayinters.size();
		for (int i = 0; i < s; i++) {
			Color lc;
			Vec3 s_vec;
			Ray s_ray;
			bool s_hit = false;

			if (rayinters[i]->hit) {
				// Lighting
				Light *lighty = scene->finite_lights[rng.next_uint() % scene->finite_lights.size()];
				Vec3 ld;
				lc = lighty->sample(rayinters[i]->inter.p, rng.next_float(), rng.next_float(), rayinters[i]->ray.time, &ld);
				float d = ld.normalize();

				s_ray.o = rayinters[i]->inter.p;
				s_ray.d = ld;
				s_ray.time = rayinters[i]->ray.time;
				s_ray.is_shadow_ray = true;
				s_ray.has_differentials = false;
				//s_ray.min_t = rayinters[i]->ray.width(rayinters[i]->inter.t);
				s_ray.min_t = 0.01;
				s_ray.max_t = d;
				s_ray.finalize();

				s_hit = scene->intersect_ray(s_ray, NULL);


				if (!s_hit) {
					rayinters[i]->inter.n.normalize();
					float lambert = dot(ld, rayinters[i]->inter.n);
					if (lambert < 0.0) lambert = 0.0;

					lc = lc * lambert * scene->finite_lights.size();
				}
			}


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

					if (rayinters[i]->hit && !s_hit) {
						image->pixels[i2*image->channels] += lc.spectrum[0] * contrib;
						image->pixels[(i2*image->channels)+1] += lc.spectrum[1] * contrib;
						image->pixels[(i2*image->channels)+2] += lc.spectrum[2] * contrib;

						//image->pixels[i2*image->channels] += 1.0 * contrib;
						//image->pixels[(i2*image->channels)+1] += 1.0 * contrib;
						//image->pixels[(i2*image->channels)+2] += 1.0 * contrib;
					}
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

	for (uint32 i = 0; i < rayinters.size(); i++) {
		delete rayinters[i];
	}


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

