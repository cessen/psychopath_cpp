#include "config.h"
#include "numtype.h"

#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <vector>

#include "rng.hpp"
#include "scene.hpp"
#include "integrator.hpp"
#include "tracer.hpp"
#include "raster.hpp"
#include "vector.hpp"
#include "matrix.hpp"
#include "camera.hpp"
#include "bilinear.hpp"
#include "sphere.hpp"
#include "primitive.hpp"

#include "config.hpp"

#include <OpenImageIO/imageio.h>
#include <OSL/oslexec.h>
OIIO_NAMESPACE_USING

#define GAMMA 2.2
#define SPP 16
#define XRES 1280
#define YRES 720
#define ASPECT (((float32)(YRES))/XRES)
#define IMAGE_CHANNELS 3
#define NUM_RAND_PATCHES 1000
#define NUM_RAND_SPHERES 1000
#define FRAC_MB 0.1


int main(int argc, char **argv)
{
	// Print program information
	std::cout << "Psychopath v" << VERSION_MAJOR << "." << VERSION_MINOR << "." << VERSION_PATCH;
#ifdef DEBUG
	std::cout << " (DEBUG build)";
#endif
	std::cout << std::endl << std::endl;

	RNG rng(42);
	RNG rng2(865546);

	/*
	 ******************************************************
	 * Build scene
	 ******************************************************
	 */
	Scene scene;

	// Add camera
	std::vector<Matrix44> cam_mats;
	cam_mats.resize(4);

	float angle = 0 * (3.14159 / 180.0);
	Vec3 axis(0.0, 0.0, 1.0);

	cam_mats[0].translate(Vec3(0.0, 0.0, -40.0));
	cam_mats[0].rotate(0.0, axis);
	cam_mats[0].translate(Vec3(0.0, 0.0, 20.0));

	cam_mats[1].translate(Vec3(0.0, 0.0, -40.0));
	cam_mats[1].rotate((angle/3)*1, axis);
	cam_mats[1].translate(Vec3(0.0, 0.0, 20.0));

	cam_mats[2].translate(Vec3(0.0, 0.0, -40.0));
	cam_mats[2].rotate((angle/3)*2, axis);
	cam_mats[2].translate(Vec3(0.0, 0.0, 20.0));

	cam_mats[3].translate(Vec3(0.0, 0.0, -40.0));
	cam_mats[3].rotate((angle/3)*3, axis);
	cam_mats[3].translate(Vec3(0.0, 0.0, 20.0));

#define LENS_DIAM 1.0
#define FOCUS_DISTANCE 40.0
#define FOV 55
	scene.camera = new Camera(cam_mats, (3.14159/180.0)*FOV, LENS_DIAM, FOCUS_DISTANCE);


	// Add random patches
	std::cout << "Generating random patches...";
	std::cout.flush();

	Bilinear *patch;
	for (int i=0; i < NUM_RAND_PATCHES; i++) {
		float32 x, y, z;
		z = 15 + (rng.next_float() * NUM_RAND_PATCHES / 4);
		float32 s = z / 15;
		x = rng.next_float_c() * 40;
		y = rng.next_float_c() * 20;

		// Motion?
		int ms = 1;
		if (rng.next_float() < FRAC_MB)
			ms = 2;

		// Flipped?
		bool flip = false;
		if (rng.next_float() < 0.5)
			flip = true;

		patch = new Bilinear(ms);

		for (int j = 0; j < ms; j++) {
			x += (rng.next_float_c() * j * 4) / s;
			y += (rng.next_float_c() * j * 4) / s;
			z += (rng.next_float_c() * j * 4) / s;

			if (flip) {
				patch->add_time_sample(j,
				                       Vec3((x*s)+2, (y*s)+2, z+(rng.next_float_c()*2)),
				                       Vec3((x*s)+2, (y*s)-2, z+(rng.next_float_c()*2)),
				                       Vec3((x*s)-2, (y*s)-2, z+(rng.next_float_c()*2)),
				                       Vec3((x*s)-2, (y*s)+2, z+(rng.next_float_c()*2)));
			} else {
				patch->add_time_sample(j,
				                       Vec3((x*s)+2, (y*s)+2, z+(rng.next_float_c()*2)),
				                       Vec3((x*s)-2, (y*s)+2, z+(rng.next_float_c()*2)),
				                       Vec3((x*s)-2, (y*s)-2, z+(rng.next_float_c()*2)),
				                       Vec3((x*s)+2, (y*s)-2, z+(rng.next_float_c()*2)));

			}
		}

		scene.add_primitive(patch);
	}
	std::cout << " done." << std::endl;
	std::cout.flush();


	// Add random spheres
	std::cout << "Generating random spheres...";
	std::cout.flush();

	const float32 radius = 0.2;

	Sphere *sphere;
	for (int i=0; i < NUM_RAND_SPHERES; i++) {
		float32 x, y, z;
		z = 15 + (rng.next_float() * NUM_RAND_SPHERES / 4) * radius;
		float32 s = z / 15;
		x = rng.next_float_c() * 40;
		y = rng.next_float_c() * 20;

		// Motion?
		int ms = 1;
		if (rng.next_float() < FRAC_MB)
			ms = 2;

		sphere = new Sphere(ms);

		for (int j = 0; j < ms; j++) {
			x += (rng.next_float_c() * j * 4) / s;
			y += (rng.next_float_c() * j * 4) / s;
			z += (rng.next_float_c() * j * 4) / s;

			sphere->add_time_sample(j,
			                        Vec3(x*s, y*s, z+(rng.next_float_c()*2)),
			                        1.0f);
		}

		scene.add_primitive(sphere);
	}
	std::cout << " done." << std::endl;
	std::cout.flush();

	std::cout << "Building acceleration structure... ";
	std::cout.flush();
	scene.finalize();
	std::cout << " done." << std::endl;
	std::cout.flush();
	//return 0;


	/*
	 ******************************************************
	 * Generate image
	 ******************************************************
	 */

	Raster *image = new Raster(XRES, YRES, IMAGE_CHANNELS, -1.0, -ASPECT, 1.0, ASPECT);
	Tracer tracer(&scene);
	Integrator integrator(&scene, &tracer, image, SPP);
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
	ImageOutput *out = ImageOutput::create(".png");
	if (!out)
		return -1;

	ImageSpec spec(image->width, image->height, IMAGE_CHANNELS, TypeDesc::UINT8);

	out->open("test.png", spec);
	out->write_image(TypeDesc::FLOAT, image->pixels);
	out->close();

	// Cleanup
	delete out;
	delete image;

	std::cout << std::endl << "Struct sizes:" << std::endl;
	std::cout << "Ray: " << sizeof(Ray) << std::endl;
	std::cout << "BBounds: " << sizeof(BBox) << std::endl;
	std::cout << "BBox: " << sizeof(BBoxT) << std::endl;
	std::cout << "BVHNode: " << sizeof(BVHNode) << std::endl;
	std::cout << "Grid: " << sizeof(Grid) << std::endl;
	std::cout << "GridBVHNode: " << sizeof(GridBVHNode) << std::endl;
	std::cout << "Primitive *: " << sizeof(Primitive *) << std::endl;
	std::cout << "TimeBox<int32>: " << sizeof(std::vector<int32>) << std::endl;
	std::cout << "std::vector<int32>: " << sizeof(std::vector<int32>) << std::endl;

	return 0;
}
