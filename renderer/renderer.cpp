#include "renderer.hpp"

#include "numtype.h"
#include "functor.hpp"
#include <OpenImageIO/imageio.h>

#include "rng.hpp"
#include "integrator.hpp"
#include "vis_integrator.hpp"
#include "direct_lighting_integrator.hpp"
#include "path_trace_integrator.hpp"
#include "tracer.hpp"
#include "scene.hpp"
#include "film.hpp"

#include "config.hpp"

#define GAMMA 2.2

struct ImageWriter: public Functor {
	Film<Color> *image;
	std::string path;

	ImageWriter(Film<Color> *im_, std::string path_) {
		image = im_;
		path = path_;
	}

	virtual void operator()() {
		// Gamma correction + dithering(256)
		uint8 *im = image->scanline_image_8bbc(2.2);

		// Save image
		OpenImageIO::ImageOutput *out = OpenImageIO::ImageOutput::create(".png");
		if (!out) {
			return;
		}
		OpenImageIO::ImageSpec spec(image->width, image->height, 3, OpenImageIO::TypeDesc::UINT8);
		out->open(path, spec);
		out->write_image(OpenImageIO::TypeDesc::UINT8, im);
		out->close();

		// Cleanup
		delete out;
		delete [] im;
	}
};

bool Renderer::render(int thread_count)
{
	RNG rng(123456);
	Film<Color> *image = new Film<Color>(res_x, res_y,
	                                     -1.0, -(((float32)(res_y))/res_x),
	                                     1.0, (((float32)(res_y))/res_x));

	ImageWriter image_writer = ImageWriter(image, output_path);

	// Render
	Tracer tracer(scene, thread_count);
	PathTraceIntegrator integrator(scene, &tracer, image, spp, thread_count, &image_writer);
	//PathTraceIntegrator integrator(scene, &tracer, image, spp, thread_count);
	//DirectLightingIntegrator integrator(scene, &tracer, image, spp, thread_count, &image_writer);
	//VisIntegrator integrator(scene, &tracer, image, spp, thread_count, &image_writer);
	integrator.integrate();

	// Save image
	image_writer();

	// Print statistics
	std::cout << "Splits during rendering: " << Config::split_count << std::endl;
	std::cout << "Micropolygons generated during rendering: " << Config::upoly_gen_count << std::endl;
	std::cout << "Grid cache misses during rendering: " << Config::cache_misses << std::endl;
	std::cout << "Primitive-ray tests during rendering: " << Config::primitive_ray_tests << std::endl;
	const uint_i grid_res = Config::grid_size_accum / Config::grid_count;
	std::cout << "Average grid resolution: " <<  grid_res << "x" << grid_res << std::endl;

	// Finished
	return true;
}
