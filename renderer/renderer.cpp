#include "renderer.hpp"

#include <functional>
#include <memory>

#include <OpenImageIO/imageio.h>

#include "numtype.h"

#include "timer.hpp"

#include "rng.hpp"
#include "integrator.hpp"
#include "path_trace_integrator.hpp"
#include "tracer.hpp"
#include "scene.hpp"
#include "film.hpp"

#include "config.hpp"
#include "global.hpp"

#define GAMMA 2.2

void write_png_from_film(Film *image, std::string path, float min_time=4.0) {
	static Timer<> timer;

	if ((timer.time() > min_time || min_time == 0.0f) && !Config::no_output) {
		timer.reset();

		// Convert to dithered sRGB
		std::vector<uint8_t> im {image->scanline_image_8bbc()};
		// Save image
		std::unique_ptr<OpenImageIO::ImageOutput> out {OpenImageIO::ImageOutput::create(".png")};
		if (!out) {
			return;
		}
		OpenImageIO::ImageSpec spec(image->width, image->height, 3, OpenImageIO::TypeDesc::UINT8);
		out->open(path, spec);
		out->write_image(OpenImageIO::TypeDesc::UINT8, &(im[0]));
		out->close();
	}
}


bool Renderer::render(int thread_count) {
	Timer<> timer; // Start timer

	// Clear rendering statistics
	Global::Stats::clear();

	RNG rng;
	std::unique_ptr<Film> image {new Film(res_x, res_y,
		                                      -1.0, -((static_cast<float>(res_y))/res_x),
		                                      1.0, ((static_cast<float>(res_y))/res_x))
	};
	image->si_x1 = subimage_x1;
	image->si_y1 = subimage_y1;
	image->si_x2 = subimage_x2;
	image->si_y2 = subimage_y2;

	// Save blank image before rendering
	write_png_from_film(image.get(), output_path, 0.0f);

	// Image writer callback
	std::function<void()> image_writer = std::bind(write_png_from_film, image.get(), output_path, 10.0);

	PathTraceIntegrator integrator(scene.get(), image.get(), spp, spp_max, variance_max, seed, thread_count, image_writer);

	std::cout << "Integrator prep time (seconds): " << timer.time() << std::endl;
	timer.reset();

	std::cout << "Rendering" << std::flush;
	integrator.integrate();
	std::cout << std::endl;


	// Save image
	write_png_from_film(image.get(), output_path, 0.0f);

#if 0
	// Print statistics

	std::cout << "Rays shot while rendering: " << Global::Stats::rays_shot << std::endl;
#ifdef GLOBAL_STATS_TOP_LEVEL_BVH_NODE_TESTS
	std::cout << "Top-level BVH node tests: " << Global::Stats::top_level_bvh_node_tests << std::endl;
#endif
	std::cout << "Primitive-ray tests during rendering: " << Global::Stats::primitive_ray_tests << std::endl;
	std::cout << "Splits during rendering: " << Global::Stats::split_count << std::endl;
	std::cout << "MicroSurface cache misses during rendering: " << Global::Stats::cache_misses << std::endl;
	std::cout << "NaN's encountered: " <<  Global::Stats::nan_count << std::endl;
	std::cout << "Bad Inf's encountered: " <<  Global::Stats::inf_count << std::endl;
#endif

	std::cout << "Render time (seconds): " << timer.time() << std::endl;


	// Finished
	return true;
}
