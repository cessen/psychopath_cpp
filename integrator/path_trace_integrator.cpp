#include "path_trace_integrator.hpp"

#include <iostream>
#include <limits>
#include <assert.h>
#include <cmath>
#include <vector>

#include "utils.hpp"
#include "monte_carlo.hpp"
#include "image_sampler.hpp"
#include "film.hpp"
#include "intersection.hpp"
#include "tracer.hpp"
#include "mis.hpp"
#include "config.hpp"

#include "surface_closure.hpp"

#include "job_queue.hpp"

#include "light.hpp"

#include "hilbert.hpp"


void PathTraceIntegrator::integrate()
{
	// Auto-calculate bucket_size
	const int min_bucket_size = 1;
	const int max_bucket_size = std::sqrt((image->width * image->height) / (thread_count * 4.0f));  // Roughly four buckets per thread
	int bucket_size = std::sqrt(static_cast<float>(Config::samples_per_bucket) / spp);
	bucket_size = std::min(max_bucket_size, bucket_size);
	bucket_size = std::max(min_bucket_size, bucket_size);

	total_items = std::ceil(float(image->width) / bucket_size) * std::ceil(float(image->height) / bucket_size);

	// Start the rendering threads
	std::vector<std::thread> threads(thread_count);
	for (auto& t: threads) {
		t = std::thread(&PathTraceIntegrator::render_blocks, this);
	}

	// Populate the bucket jobs
	uint32_t i = 0;
	uint32_t x = 0;
	uint32_t y = 0;
	const int morton_stop = std::max(image->width, image->height) * 2;
	const bool greater_width = image->width > image->height;
	while (true) {
		if (greater_width)
			Morton::d2xy(i, &y, &x);
		else
			Morton::d2xy(i, &x, &y);
		const int xp = x * bucket_size;
		const int yp = y * bucket_size;

		if (xp < image->width && yp < image->height) {
			const int w = std::min(image->width - xp, bucket_size);
			const int h = std::min(image->height - yp, bucket_size);
			blocks.push_blocking( {xp,yp,w,h});
		}

		if (xp >= morton_stop && yp >= morton_stop)
			break;

		++i;
	}
	blocks.disallow_blocking();

	// Wait for render threads to finish
	for (auto& t: threads) {
		t.join();
	}

	std::cout << std::flush;
}


/*
 * Initializes the state of a path.
 */
void PathTraceIntegrator::init_path(PTState* pstate, Sampler s, short x, short y)
{
	*pstate = PTState();
	pstate->sampler = s;
	pstate->pix_x = x;
	pstate->pix_y = y;
}


/*
 * Calculate the next ray the path needs to shoot.
 */
WorldRay PathTraceIntegrator::next_ray_for_path(const WorldRay& prev_ray, PTState* pstate)
{
	PTState& path = *pstate; // Shorthand for the passed path
	WorldRay ray;

	if (path.step == 0) {
		const static float PIXEL_FILTER_WIDTH = 1.5f;
		const float samp_x = (logit(path.sampler.next(), PIXEL_FILTER_WIDTH) + 0.5f + path.pix_x) / image->width;
		const float samp_y = (logit(path.sampler.next(), PIXEL_FILTER_WIDTH) + 0.5f + path.pix_y) / image->height;

		// Camera ray
		const float rx = (samp_x - 0.5) * (image->max_x - image->min_x);
		const float ry = (0.5 - samp_y) * (image->max_y - image->min_y);
		const float dx = (image->max_x - image->min_x) / image->width;
		const float dy = (image->max_y - image->min_y) / image->height;

		const float samp_a = path.sampler.next();
		const float samp_b = path.sampler.next();
		const float samp_c = path.sampler.next();
		ray = scene->camera->generate_ray(rx, ry, dx, dy, samp_c, samp_a, samp_b);
		path.time = ray.time;
	} else if (path.step % 2) {
		// Shadow ray

		// BSDF
		SurfaceClosure* bsdf = path.inter.surface_closure.get();

		if (!bsdf->is_delta()) {
			// Get differential geometry of hit point in world space
			const DifferentialGeometry geo = path.inter.geo.transformed_from(path.inter.space);

			// Get the ray origin offset
			// TODO: this needs to take into account the BSDF (e.g. transmittance vs reflectance)
			Vec3 pos_offset = path.inter.space.dir_from(path.inter.offset);
			if (path.inter.backfacing)
				pos_offset *= -1.0f;

			// TEMPORARY
			// TODO: the surface normal passed to the light query should be
			// determined based on the BSDF
			Vec3 lq_nor = geo.n;
			if (path.inter.backfacing)
				lq_nor *= -1.0f;

			// Get a sample from lights in the scene
			LightQuery lq {path.sampler.next(), path.sampler.next(), path.sampler.next(), 0.0f,
			               geo.p, lq_nor, path.time,
			               Transform()
			              };
			lq.pdf = 1.0f;
			scene->root->light_accel.sample(&lq);

			// Get the pdf of sampling this light vector from the bsdf
			//const float bsdf_pdf = bsdf->pdf(Vec3(), lq.to_light, path.inter);

			// Set light color
			//path.lcol = (lq.color * power_heuristic(lq.pdf, bsdf_pdf) / lq.pdf) * scene->root->light_accel.light_count();
			path.lcol = (lq.color / lq.pdf) * scene->root->light_accel.light_count();

			// Create a shadow ray for this path
			ray.o = geo.p + pos_offset;
			ray.d = lq.to_light - pos_offset;
			ray.time = path.time;
			ray.type = WorldRay::OCCLUSION;

			// Propagate ray differentials
			bsdf->propagate_differentials(path.inter.t, path.prev_ray, geo, &ray);
		} else {
			path.lcol = Color(0.0f);
		}
	} else {
		// Bounce ray

		SurfaceClosure* bsdf = path.inter.surface_closure.get();

		// Get differential geometry of hit point in world space
		const DifferentialGeometry geo = path.inter.geo.transformed_from(path.inter.space);

		// Get the ray origin offset
		// TODO: this needs to take into account the BSDF (e.g. transmittance vs reflectance)
		Vec3 pos_offset = path.inter.space.dir_from(path.inter.offset);
		if (path.inter.backfacing)
			pos_offset *= -1.0f;

		Vec3 out;
		Color filter;
		float pdf;

		bsdf->sample(path.prev_ray.d, geo, path.sampler.next(), path.sampler.next(), &out, &filter, &pdf);

		ray.o = geo.p + pos_offset;
		ray.d = out;
		ray.time = path.time;
		ray.type = WorldRay::R_DIFFUSE;

		// Propagate ray differentials
		bsdf->propagate_differentials(path.inter.t, path.prev_ray, geo, &ray);

		// Calculate the color filtering effect that the
		// bounce from the current intersection will create.
		if (!bsdf->is_delta()) {
			if (pdf < 0.00001f)
				pdf = 0.00001f; // Dodge 0 pdf's that might slip through
			path.fcol *= filter / pdf;
		} else {
			path.fcol *= filter;
		}
	}

	return ray;
}


/*
 * Update the path based on the result of a ray shot
 */
void PathTraceIntegrator::update_path(PTState* pstate, const WorldRay& ray, const Intersection& inter)
{
	PTState& path = *pstate; // Shorthand for the passed path

	if (path.step % 2) {
		// Result of shadow ray
		if (!inter.hit) {
			// Sample was lit
			SurfaceClosure* bsdf = path.inter.surface_closure.get();

			if (!bsdf->is_delta()) {
				const DifferentialGeometry geo = path.inter.geo.transformed_from(path.inter.space);

				Color fac = bsdf->evaluate(path.prev_ray.d, ray.d, geo);

				path.col += path.fcol * path.lcol * fac;
			}
		}
	} else {
		// Result of bounce or camera ray
		if (inter.hit) {
			// Ray hit something!
			if (auto emit_closure = dynamic_cast<const EmitClosure*>(inter.surface_closure.get())) {
				// Hit emitting surface, handle specially
				// Ray didn't hit anything
				path.done = true;
				path.col += path.fcol * emit_closure->emitted_color();
			} else {
				path.inter = inter; // Store intersection data for creating shadow ray
				path.prev_ray = ray;  // Store incoming ray direction for use in shading calculations
			}
		} else {
			// Ray didn't hit anything
			path.done = true;
			path.col += path.fcol * scene->background_color;
		}
	}

	path.step++;

	// Has the path hit its maximum length?
	if (path.step == (path_length * 2))
		path.done = true;
}


void PathTraceIntegrator::render_blocks()
{
	PixelBlock pb {0,0,0,0};
	RNG rng;
	ImageSampler image_sampler(spp, image->width, image->height, seed);
	Tracer tracer(scene);

	// Light path array
	std::vector<PTState> paths;

	// Ray and Intersection arrays
	std::vector<WorldRay> rays;
	std::vector<Intersection> intersections;

	// Keep rendering blocks as long as they exist in the queue
	while (blocks.pop_blocking(&pb)) {
		// Seed tracer for random numbers
		tracer.set_seed(seed + (pb.x ^ (pb.y << 16 | pb.y >> 16) ^ pb.w ^ (pb.h << 16 | pb.h >> 16)));

		Color max_variance = Color(9999999999.0f);
		float samp_it = 0;

		while (max_variance[0] > image_variance_max && samp_it < spp_max) {
			const size_t sample_count = (pb.h * pb.w) * spp;

			// Resize arrays for the apropriate sample count
			paths.resize(sample_count);
			rays.resize(sample_count);
			intersections.resize(sample_count);

			// Generate samples and corresponding paths
			int samp_i = 0;
			for (int x = pb.x; x < (pb.x + pb.w); ++x) {
				for (int y = pb.y; y < (pb.y + pb.h); ++y) {
					for (int s = 0; s < spp; ++s) {
						init_path(&paths[samp_i], image_sampler.get_single_sampler(x, y, s), x, y);
						++samp_i;
					}
				}
			}
			samp_it += spp;

			auto p_begin = &(*paths.begin());
			auto p_end = &(*paths.end());

			// Path tracing loop for the paths we have
			while (p_begin != p_end) {
				int path_count = std::distance(p_begin, p_end);

				// Size the ray buffer and intersection buffers appropriately
				rays.resize(path_count);
				intersections.resize(path_count);

				// Create path rays
				for (int i = 0; i < path_count; ++i) {
					rays[i] = next_ray_for_path(rays[i], p_begin+i);
				}

				// Trace rays
				tracer.trace(&(*rays.begin()), &(*rays.end()), &(*intersections.begin()), &(*intersections.end()));

				// Update paths based on result
				for (int i = 0; i < path_count; ++i) {
					update_path(p_begin+i, rays[i], intersections[i]);
				}

				// Partition paths based on which ones are active
				p_begin = std::partition(p_begin, p_end, [this](const PTState& path) {
					return path.done;
				});
			}


			if (!Config::no_output) {
				// Accumulate the samples

				for (uint32_t i = 0; i < paths.size(); i++) {
					image->add_sample(paths[i].col, paths[i].pix_x, paths[i].pix_y);
				}

				// Callback
				if (callback) {
					image_mut.lock();
					callback();
					image_mut.unlock();
				}
			}

			max_variance = Color(0.0f);
			for (int x = pb.x; x < (pb.x + pb.w); ++x) {
				for (int y = pb.y; y < (pb.y + pb.h); ++y) {
					max_variance = mmax(max_variance, image->variance_estimate(x,y));
				}
			}

		}

		// Update render progress
		progress_lock.lock();
		++completed_items;
		print_progress();
		progress_lock.unlock();
	}
}
