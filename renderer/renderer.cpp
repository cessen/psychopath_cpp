#include "renderer.hpp"

#include "numtype.h"
#include "functor.hpp"
#include <OpenImageIO/imageio.h>

#include "rng.hpp"
#include "integrator.hpp"
#include "direct_lighting_integrator.hpp"
#include "path_trace_integrator.hpp"
#include "tracer.hpp"
#include "scene.hpp"
#include "film.hpp"

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
	//PathTraceIntegrator integrator(scene, &tracer, image, spp, thread_count, &image_writer);
	PathTraceIntegrator integrator(scene, &tracer, image, spp, thread_count);
	integrator.integrate();

	// Save image
	image_writer();

	// Finished
	return true;
}
