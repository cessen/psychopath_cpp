#include "path_trace_integrator.hpp"

#include <iostream>
#include <limits>
#include <assert.h>
#include <cmath>
#include "image_sampler.hpp"
#include "film.hpp"
#include "intersection.hpp"
#include "tracer.hpp"
#include "config.hpp"

#include "job_queue.hpp"
#include "array.hpp"

#include "light.hpp"

#include "hilbert.hpp"


float lambert(Vec3 v1, Vec3 v2)
{
	v1.normalize();
	v2.normalize();
	float f = dot(v1, v2);
	if (f < 0.0f)
		f = 0.0f;
	return f;
}

class Lambert
{
public:
	Color col {1.0f};

	void sample(const WorldRay &in, const Intersection &inter, const float &su, const float &sv,
	            WorldRay *out, Color *filter, float *pdf) {
		const Vec3 nn = inter.n.normalized();
		const Vec3 nns = (!inter.backfacing) ? nn : (nn * -1.0f); // Shading normal, flip for backfacing

		// Generate a random ray direction in the hemisphere
		// of the surface.
		Vec3 dir = cosine_sample_hemisphere(su, sv);
		*pdf = dir.z * 2;

		if (*pdf < 0.001)
			*pdf = 0.001;
		dir = zup_to_vec(dir, nns);

		*filter = col * lambert(dir, nns);

		if (dot(nn, dir.normalized()) >= 0.0f)
			out->o = inter.p + inter.offset;
		else
			out->o = inter.p + (inter.offset * -1.0f);
		out->d = dir;
		out->time = in.time;
		out->type = Ray::R_DIFFUSE;

		// Ray differentials
		// TODO: do this correctly
		out->odx = Vec3(1,0,0) * inter.owp();
		out->ody = Vec3(0,1,0) * inter.owp();
		out->ddx = Vec3(1,0,0) * 0.15;
		out->ddy = Vec3(0,1,0) * 0.15;
		/*
		ray.odx = inter.pdx();
		ray.ddx = ray.odx.normalized() * inter.ddx.length();
		ray.ody = inter.pdy();
		ray.ddy = ray.ody.normalized() * inter.ddy.length();
		*/
	}

	void propagate_differentials(const WorldRay& in, const Intersection& inter, WorldRay* out) {
		const float len = out->d.length();

		// TODO: do this correctly
		out->odx = Vec3(1,0,0) * inter.owp();
		out->ody = Vec3(0,1,0) * inter.owp();
		out->ddx = Vec3(1,0,0) * (inter.dw / len);
		out->ddy = Vec3(0,1,0) * (inter.dw / len);
		/*
		ray.odx = path.inter.pdx();
		ray.ddx = ray.odx.normalized() * path.inter.ddx.length();
		ray.ody = path.inter.pdy();
		ray.ddy = ray.ody.normalized() * path.inter.ddy.length();
		*/
	}

	Color evaluate(const Vec3& in, const Vec3& out, const Intersection& inter) {
		Color lam;

		if (inter.backfacing) {
			lam = col * lambert(out, inter.n * -1.0f);
		} else {
			lam = col * lambert(out, inter.n);
		}

		return lam;
	}
};


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
void PathTraceIntegrator::init_path(PTState* pstate, const float* samps, short x, short y)
{
	*pstate = PTState();
	pstate->samples = samps;
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
		// Camera ray
		const float rx = (path.samples[0] - 0.5) * (image->max_x - image->min_x);
		const float ry = (0.5 - path.samples[1]) * (image->max_y - image->min_y);
		const float dx = (image->max_x - image->min_x) / image->width;
		const float dy = (image->max_y - image->min_y) / image->height;
		ray = scene->camera->generate_ray(rx, ry, dx, dy, path.samples[4], path.samples[2], path.samples[3]);
		path.time = ray.time;

		// Increment the sample pointer
		path.samples += 5;
	} else if (path.step % 2) {
		// Shadow ray

		// Calculate the surface normal facing in the direction of where the ray hit came from
		Vec3 nor = path.inter.n;
		if (dot(nor, path.inter.in.normalized()) > 0.0f) {
			nor *= -1.0f;
		}

		// Select a light and store the normalization factor for it's output
		std::tuple<Light*, float> light_select = scene->finite_light_accel.sample(path.inter.p, nor, path.samples[0]);
		Light* lighty = std::get<0>(light_select);
		const float inv_probability = 1.0f / std::get<1>(light_select);

		// Sample the light source
		Vec3 ld;
		path.lcol = lighty->sample(path.inter.p, path.samples[1], path.samples[2], path.time, &ld) * inv_probability;

		// Create a shadow ray for this path
		if (dot(path.inter.n.normalized(), ld.normalized()) >= 0.0f)
			ray.o = path.inter.p + path.inter.offset;
		else
			ray.o = path.inter.p + (path.inter.offset * -1.0f);
		ray.d = ld;
		ray.time = path.time;
		ray.type = Ray::OCCLUSION;

		// Propagate ray differentials
		Lambert bsdf;
		bsdf.propagate_differentials(prev_ray, path.inter, &ray);

		// Increment the sample pointer
		path.samples += 3;
	} else {
		// Bounce ray

		Lambert bsdf;
		Color filter;
		float pdf;

		bsdf.sample(prev_ray, path.inter, path.samples[0], path.samples[1], &ray, &filter, &pdf);

		// Calculate the color filtering effect that the
		// bounce from the current intersection will create.
		// TODO: use actual shaders here.
		path.fcol *= filter / pdf;

		// Increment the sample pointer
		path.samples += 2;
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
			Lambert bsdf;

			Color lam = bsdf.evaluate(Vec3(), ray.d, path.inter);

			path.col += path.fcol * path.lcol * lam;
		}
	} else {
		// Result of bounce or camera ray
		if (inter.hit) {
			// Ray hit something!
			path.inter = inter; // Store intersection data for creating shadow ray
		} else {
			// Ray didn't hit anything
			path.done = true;
			path.col += path.fcol * Color(0.0f); // Background color
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

	const size_t samp_dim = 5 + (path_length * 5);

	// Sample array
	Array<float> samps;

	// Light path array
	Array<PTState> paths;

	// Ray and Intersection arrays
	Array<WorldRay> rays;
	Array<Intersection> intersections;

	// Keep rendering blocks as long as they exist in the queue
	while (blocks.pop_blocking(&pb)) {
		const size_t sample_count = (pb.h * pb.w) * spp;

		// Resize arrays for the apropriate sample count
		samps.resize(sample_count * samp_dim);
		paths.resize(sample_count);
		rays.resize(sample_count);
		intersections.resize(sample_count);

		// Generate samples and corresponding paths
		int samp_i = 0;
		for (int x = pb.x; x < (pb.x + pb.w); ++x) {
			for (int y = pb.y; y < (pb.y + pb.h); ++y) {
				for (int s = 0; s < spp; ++s) {
					image_sampler.get_sample(x, y, s, samp_dim, &(samps[samp_i*samp_dim]));
					init_path(paths.begin() + samp_i, samps.begin() + (samp_i*samp_dim), x, y);
					++samp_i;
				}
			}
		}

		uint32_t samp_size = samps.size() / samp_dim;

		auto p_begin = paths.begin();
		auto p_end = paths.end();

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
			tracer.trace(Slice<WorldRay>(rays), Slice<Intersection>(intersections));

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

			for (uint32_t i = 0; i < samp_size; i++) {
				image->add_sample(paths[i].col, paths[i].pix_x, paths[i].pix_y);
			}

			// Callback
			if (callback) {
				image_mut.lock();
				callback();
				image_mut.unlock();
			}
		}

		// Update render progress
		progress_lock.lock();
		++completed_items;
		print_progress();
		progress_lock.unlock();
	}
}

