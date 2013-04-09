#include "path_trace_integrator.hpp"

#include <iostream>
#include <limits>
#include <assert.h>
#include "image_sampler.hpp"
#include "film.hpp"
#include "intersection.hpp"
#include "tracer.hpp"
#include "config.hpp"

#include "job_queue.hpp"
#include "array.hpp"

#include "light.hpp"

#include "hilbert.hpp"

#define RAYS_AT_A_TIME 1000000


float32 lambert(Vec3 v1, Vec3 v2)
{
	v1.normalize();
	v2.normalize();
	float32 f = dot(v1, v2);
	if (f < 0.0f)
		f = 0.0f;
	return f;
}

/*
 * A path tracing path.
 * Stores state of a path in progress.
 */
struct PTPath {
	Intersection inter;
	Color col; // Color of the sample collected so far
	Color fcol; // Accumulated filter color from light path
	Color lcol; // Temporary storage for incoming light color

	bool done;
};


void create_camera_rays(Camera *camera, Film<Color> *image, Array<Ray> *rays_, Array<float32> *samps_, Array<uint32> *ids_, uint_i samp_dim, uint_i first, uint_i last)
{
	Array<Ray> &rays = *rays_;
	Array<float32> &samps = *samps_;
	Array<uint32> &ids = *ids_;

	for (uint32 i = first; i < last; i++) {
		float32 rx = (samps[i*samp_dim] - 0.5) * (image->max_x - image->min_x);
		float32 ry = (0.5 - samps[i*samp_dim + 1]) * (image->max_y - image->min_y);
		float32 dx = (image->max_x - image->min_x) / image->width;
		float32 dy = (image->max_y - image->min_y) / image->height;
		rays[i] = camera->generate_ray(rx, ry, dx, dy, samps[i*samp_dim+4], samps[i*samp_dim+2], samps[i*samp_dim+3]);
		rays[i].finalize();
		ids[i] = i;
	}
}


void PathTraceIntegrator::integrate()
{
	const uint_i samp_dim = 5 + (path_length * 5);

	RNG rng;
	ImageSampler image_sampler(spp, image->width, image->height, seed);

	// Sample array
	Array<float32> samps;
	samps.resize(RAYS_AT_A_TIME * samp_dim);

	// Coordinate array
	Array<uint16> coords;
	coords.resize(RAYS_AT_A_TIME * 2);

	// Light path array
	Array<PTPath> paths;
	paths.resize(RAYS_AT_A_TIME);

	// Ray and Intersection arrays
	Array<Ray> rays(RAYS_AT_A_TIME);
	Array<Intersection> intersections(RAYS_AT_A_TIME);

	// ids corresponding to the rays
	Array<uint32> ids(RAYS_AT_A_TIME);

	// Go for it!
	bool last = false;
	while (true) {
		// Generate samples
		std::cout << "\t--------\n\tGenerating samples" << std::endl;
		for (int i = 0; i < RAYS_AT_A_TIME; i++) {
			paths[i].done = false;
			paths[i].col = Color(0.0);
			paths[i].fcol = Color(1.0);

			if (!image_sampler.get_next_sample(samp_dim, &(samps[i*samp_dim]), &(coords[i*2]))) {
				samps.resize(i*samp_dim);
				paths.resize(i);
				last = true;
				break;
			}
		}
		uint32 samp_size = samps.size() / samp_dim;

		// Path tracing loop for the samples we have
		for (int path_n=0; path_n < path_length; path_n++) {
			// Size the ray buffer appropriately
			rays.resize(samp_size);
			intersections.resize(samp_size);

			int32 so = path_n * 5; // Sample offset

			// Create path rays
			std::cout << "\tGenerating path rays" << std::endl;
			if (path_n == 0) {
				// First segment of path is camera rays
				JobQueue<> jq(thread_count);
				const uint_i job_size = rays.size() / thread_count;
				for (uint_i i = 0; i < rays.size(); i += job_size) {
					uint_i end = i + job_size;
					if (end > rays.size())
						end = rays.size();
					jq.push(std::bind(create_camera_rays, scene->camera, image, &rays, &samps, &ids, samp_dim, i, end));
				}
				jq.finish();
			} else {
				// Other path segments are bounces
				uint32 pri = 0; // Path ray index
				for (uint32 i = 0; i < samp_size; i++) {
					if (!paths[i].done) {
						const Vec3 nn = paths[i].inter.n.normalized();
						const Vec3 nns = (!paths[i].inter.backfacing) ? nn : (nn * -1.0f); // Shading normal, flip for backfacing

						// Generate a random ray direction in the hemisphere
						// of the surface.
						// TODO: use BxDF distribution here
						// TODO: use proper PDF here
						Vec3 dir = cosine_sample_hemisphere(samps[i*samp_dim+so], samps[i*samp_dim+so+1]);
						float32 pdf = dir.z * 2;

						if (pdf < 0.001)
							pdf = 0.001;
						dir = zup_to_vec(dir, nns);

						// Calculate the color filtering effect that the
						// bounce from the current intersection will create.
						// TODO: use actual shaders here.
						paths[i].fcol *= lambert(dir, nns) / pdf;

						// Set the id
						ids[pri] = i;

						// Create a bounce ray for this path
						if (dot(nn, dir.normalized()) >= 0.0f)
							rays[pri].o = paths[i].inter.p + paths[i].inter.offset;
						else
							rays[pri].o = paths[i].inter.p + (paths[i].inter.offset * -1.0f);
						rays[pri].d = dir;
						rays[pri].time = samps[i*samp_dim+4];
						rays[pri].is_shadow_ray = false;
						rays[pri].min_t = 0.0001f;
						rays[pri].max_t = std::numeric_limits<float32>::infinity();
						//rays[pri].has_differentials = true;

						// Ray differentials
						rays[pri].ow = paths[i].inter.owp();
						rays[pri].dw = 0.2;
						//rays[pri].odx = paths[i].inter.pdx();
						//rays[pri].ddx = rays[pri].odx.normalized() * paths[i].inter.ddx.length();
						//rays[pri].ody = paths[i].inter.pdy();
						//rays[pri].ddy = rays[pri].ody.normalized() * paths[i].inter.ddy.length();

						rays[pri].finalize();

						// Increment path ray index
						pri++;
					}
				}
				rays.resize(pri);
			}


			// Trace the rays
			tracer->trace(rays, &intersections);


			// Update paths
			uint32 rsize = rays.size();
			for (uint32 i = 0; i < rsize; i++) {
				const uint32 id = ids[i];
				if (intersections[i].hit) {
					// Ray hit something!  Store intersection data
					paths[id].inter = intersections[i];
				} else {
					// Ray didn't hit anything, done and black background
					paths[id].done = true;
					paths[id].col += Color(0.0f);
				}
			}


			// Generate a bunch of shadow rays
			if (scene->finite_lights.size() > 0) {
				std::cout << "\tGenerating shadow rays" << std::endl;
				uint32 sri = 0; // Shadow ray index
				for (uint32 i = 0; i < paths.size(); i++) {
					if (!paths[i].done) {
						// Select a light and store the normalization factor for it's output
						Light *lighty = scene->finite_lights[(uint32)(samps[i*samp_dim+5+so+2] * scene->finite_lights.size()) % scene->finite_lights.size()];
						//Light *lighty = scene->finite_lights[rng.next_uint() % scene->finite_lights.size()];

						// Sample the light source
						Vec3 ld;
						paths[i].lcol = lighty->sample(paths[i].inter.p, samps[i*samp_dim+5+so+3], samps[i*samp_dim+5+so+4], samps[i*samp_dim+4], &ld)
						                * (float32)(scene->finite_lights.size());
						//paths[i].lcol = lighty->sample(paths[i].inter.p, 0.0f, 0.0f, samps[i*samp_dim+4], &ld)
						//                * (float32)(scene->finite_lights.size());

						// Create a shadow ray for this path
						float d = ld.length();
						ld.normalize();
						if (dot(paths[i].inter.n.normalized(), ld) >= 0.0f)
							rays[sri].o = paths[i].inter.p + paths[i].inter.offset;
						else
							rays[sri].o = paths[i].inter.p + (paths[i].inter.offset * -1.0f);
						rays[sri].d = ld;
						rays[sri].time = samps[i*samp_dim+4];
						rays[sri].is_shadow_ray = true;
						rays[sri].min_t = 0.0001f;
						rays[sri].max_t = d;
						//rays[sri].has_differentials = true;

						// Ray differentials
						rays[sri].ow = paths[i].inter.owp();
						rays[sri].dw = paths[i].inter.dw;
						//rays[sri].odx = paths[i].inter.pdx();
						//rays[sri].ddx = rays[sri].odx.normalized() * paths[i].inter.ddx.length();
						//rays[sri].ody = paths[i].inter.pdy();
						//rays[sri].ddy = rays[sri].ody.normalized() * paths[i].inter.ddy.length();

						rays[sri].finalize();
						ids[sri] = i;

						sri++;
					}
				}
				rays.resize(sri);


				// Trace the shadow rays
				tracer->trace(rays, &intersections);


				// Calculate sample colors
				rsize = rays.size();
				for (uint32 i = 0; i < rsize; i++) {
					const uint32 id = ids[i];
					if (!intersections[i].hit) {
						// Sample was lit
						// TODO: use actual shaders here
						float lam = 0.0f;
						if (paths[id].inter.backfacing)
							lam = lambert(rays[i].d, paths[id].inter.n * -1.0f);
						else
							lam = lambert(rays[i].d, paths[id].inter.n);
						paths[id].col += paths[id].fcol * paths[id].lcol * lam;
					}
				}
			}
		}

		// Accumulate the samples
		std::cout << "\tAccumulating samples" << std::endl;
		for (uint32 i = 0; i < samp_size; i++) {
			image->add_sample(paths[i].col, coords[i*2], coords[i*2+1]);
		}

		// Print percentage complete
		float perc = image_sampler.percentage() * 100;
		uint32 pr = std::cout.precision();
		std::cout.precision(4);
		std::cout << perc << "%" << std::endl;
		std::cout.precision(pr);

		// Callback
		if (callback)
			callback();

		if (last)
			break;
	}
}

