#include "renderer.hpp"

#include <functional>
#include <memory>

#include <OpenImageIO/imageio.h>

#include "numtype.h"

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

void write_png_from_film(Film<Color> *image, std::string path)
{
	// Gamma correction + dithering(256)
	std::vector<uint8_t> im {image->scanline_image_8bbc(2.2)};

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


bool Renderer::render(int thread_count)
{
	RNG rng;
	std::unique_ptr<Film<Color>> image {new Film<Color>(res_x, res_y,
		        -1.0, -((static_cast<float>(res_y))/res_x),
		        1.0, ((static_cast<float>(res_y))/res_x))
	};


	std::function<void()> image_writer = std::bind(write_png_from_film, image.get(), output_path);

	// Render
	std::vector<Tracer> tracers(thread_count, Tracer(scene.get()));
	PathTraceIntegrator integrator(scene.get(), tracers, image.get(), spp, seed, thread_count, image_writer);
	//PathTraceIntegrator integrator(scene, &tracer, image.get(), spp, seed, thread_count);
	//DirectLightingIntegrator integrator(scene, &tracer, image.get(), spp, seed, thread_count, image_writer);
	//VisIntegrator integrator(scene, &tracer, image.get(), spp, thread_count, seed, image_writer);
	integrator.integrate();

	// Save image
	image_writer();

	// Print statistics
	std::cout << "Primitive-ray tests during rendering: " << Global::Stats::primitive_ray_tests << std::endl;
	std::cout << "Splits during rendering: " << Global::Stats::split_count << std::endl;
	std::cout << "MicroSurface cache misses during rendering: " << Global::Stats::cache_misses << std::endl;
	std::cout << "MicroSurfaces generated during rendering: " << Global::Stats::microsurface_count << std::endl;
	std::cout << "MicroSurface elements generated during rendering: " << Global::Stats::microelement_count << std::endl;
	std::cout << "Average MicroSurface elements per MicroSurface: " <<  Global::Stats::microelement_count / static_cast<float>(Global::Stats::microsurface_count) << std::endl;
	std::cout << "Minimum MicroSurface elements per MicroSurface: " <<  Global::Stats::microelement_min_count << std::endl;
	std::cout << "Maximum MicroSurface elements per MicroSurface: " <<  Global::Stats::microelement_max_count << std::endl;

	std::cout << "NaN's encountered: " <<  Global::Stats::nan_count << std::endl;
	std::cout << "Bad Inf's encountered: " <<  Global::Stats::inf_count << std::endl;


	// Finished
	return true;
}
