#include "renderer.hpp"

#include "numtype.h"
#include <OpenImageIO/imageio.h>

#include "rng.hpp"
#include "integrator.hpp"
#include "direct_lighting_integrator.hpp"
#include "path_trace_integrator.hpp"
#include "tracer.hpp"
#include "scene.hpp"
#include "film.hpp"

#define GAMMA 2.2

bool Renderer::render(int thread_count)
{
	RNG rng(123456);
	Film<Color> *image = new Film<Color>(res_x, res_y, 2,
	                                     -1.0, -(((float32)(res_y))/res_x),
	                                     1.0, (((float32)(res_y))/res_x));

	// Render
	Tracer tracer(scene, thread_count);
	PathTraceIntegrator integrator(scene, &tracer, image, spp, thread_count);
	integrator.integrate();

	// Gamma correction + dithering(256)
	uint8 *im = image->scanline_image_8bbc(2.2);

	// Save image
	OpenImageIO::ImageOutput *out = OpenImageIO::ImageOutput::create(".png");
	if (!out) {
		delete image;
		return false;
	}
	OpenImageIO::ImageSpec spec(image->width, image->height, 3, OpenImageIO::TypeDesc::UINT8);
	out->open(output_path, spec);
	out->write_image(OpenImageIO::TypeDesc::UINT8, im);
	out->close();

	// Cleanup
	delete out;
	delete image;
	delete [] im;

	// Finished
	return true;
}
