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
#include "global.hpp"

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
	RNG rng;
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
	std::cout << "Primitive-ray tests during rendering: " << Global::Stats::primitive_ray_tests << std::endl;
	std::cout << "Splits during rendering: " << Global::Stats::split_count << std::endl;
	std::cout << "MicroSurface cache misses during rendering: " << Global::Stats::cache_misses << std::endl;
	std::cout << "MicroSurfaces generated during rendering: " << Global::Stats::microsurface_count << std::endl;
	std::cout << "MicroSurface elements generated during rendering: " << Global::Stats::microelement_count << std::endl;
	std::cout << "Average MicroSurface elements per MicroSurface: " <<  Global::Stats::microelement_count / (float32)Global::Stats::microsurface_count << std::endl;
	std::cout << "Minimum MicroSurface elements per MicroSurface: " <<  Global::Stats::microelement_min_count << std::endl;
	std::cout << "Maximum MicroSurface elements per MicroSurface: " <<  Global::Stats::microelement_max_count << std::endl;

	// Finished
	return true;
}
