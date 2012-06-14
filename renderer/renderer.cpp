#include "renderer.hpp"

#include "numtype.h"
#include <OpenImageIO/imageio.h>

#include "rng.hpp"
#include "integrator.hpp"
#include "direct_lighting_integrator.hpp"
#include "path_trace_integrator.hpp"
#include "tracer.hpp"
#include "scene.hpp"

#define IMAGE_CHANNELS 3
#define GAMMA 2.2

bool Renderer::render(int thread_count)
{
	RNG rng(123456);

	Raster<float32> *image = new Raster<float32>(res_x, res_y, IMAGE_CHANNELS,
	        -1.0, -(((float32)(res_y))/res_x),
	        1.0, (((float32)(res_y))/res_x));
	Tracer tracer(scene, thread_count);
	PathTraceIntegrator integrator(scene, &tracer, image, spp, thread_count);
	integrator.integrate();

	int i;
	// Gamma correction + dithering(256)
	for (int y = 0; y < image->height; y++) {
		for (int x = 0; x < image->width; x++) {
			i = x + (y*image->width);

			image->pixels[(i*IMAGE_CHANNELS)] = pow(image->pixels[(i*IMAGE_CHANNELS)], 1.0/GAMMA);
			image->pixels[(i*IMAGE_CHANNELS)] += rng.next_float_c() / 256;
			image->pixels[(i*IMAGE_CHANNELS)] = image->pixels[(i*IMAGE_CHANNELS)] < 0.0 ? 0.0 : image->pixels[(i*IMAGE_CHANNELS)];

			image->pixels[(i*IMAGE_CHANNELS)+1] = pow(image->pixels[(i*IMAGE_CHANNELS)+1], 1.0/GAMMA);
			image->pixels[(i*IMAGE_CHANNELS)+1] += rng.next_float_c() / 256;
			image->pixels[(i*IMAGE_CHANNELS)+1] = image->pixels[(i*IMAGE_CHANNELS)+1] < 0.0 ? 0.0 : image->pixels[(i*IMAGE_CHANNELS)+1];

			image->pixels[(i*IMAGE_CHANNELS)+2] = pow(image->pixels[(i*IMAGE_CHANNELS)+2], 1.0/GAMMA);
			image->pixels[(i*IMAGE_CHANNELS)+2] += rng.next_float_c() / 256;
			image->pixels[(i*IMAGE_CHANNELS)+2] = image->pixels[(i*IMAGE_CHANNELS)+2] < 0.0 ? 0.0 : image->pixels[(i*IMAGE_CHANNELS)+2];
		}
	}

	// Save image
	OpenImageIO::ImageOutput *out = OpenImageIO::ImageOutput::create(".png");
	if (!out) {
		delete image;
		return false;
	}
	OpenImageIO::ImageSpec spec(image->width, image->height, IMAGE_CHANNELS, OpenImageIO::TypeDesc::UINT8);
	out->open(output_path, spec);
	out->write_image(OpenImageIO::TypeDesc::FLOAT, image->pixels);
	out->close();

	// Cleanup
	delete out;
	delete image;

	// Finished
	return true;
}
