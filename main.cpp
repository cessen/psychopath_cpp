#include "config.h"
#include "numtype.h"

#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <vector>


#include "rng.hpp"
#include "renderer.hpp"
#include "scene.hpp"
#include "vector.hpp"
#include "matrix.hpp"
#include "transform.hpp"
#include "camera.hpp"

#include "primitive.hpp"
#include "bilinear.hpp"
#include "sphere.hpp"

#include "light.hpp"
#include "point_light.hpp"
#include "sphere_light.hpp"

#include "ray.hpp"
#include "intersection.hpp"
#include "potentialinter.hpp"
#include "micro_surface.hpp"

#include "global.hpp"

#include <gperftools/profiler.h>

//#include <OSL/oslexec.h>

#include <boost/program_options.hpp>
namespace BPO = boost::program_options;

/*
 * Previous results:
 *
 * Pre-ray-reordering:
 * 1000 spheres / 0 patches
 * 1280x720 @ 4spp
 * 1 thread
 * ~26 second
 *
 * Naive-ray-reordering:
 * 1000 spheres / 0 patches
 * 1280x720 @ 4spp
 * 1 thread
 * 4 potints
 * ~43 second
 *
 * Naive-ray-reordering:
 * 1000 spheres / 0 patches
 * 1280x720 @ 4spp
 * 1 thread
 * Intermediate image writing disabled
 * 2 potints
 * 36.3 second
 *
 * Threaded ray-reordering:
 * 1000 spheres / 0 patches
 * 1280x720 @ 4spp
 * 4 threads
 * Intermediate image writing disabled
 * 2 potints
 * 24.8 second
 *
 * Threaded ray-reordering w/ counting sort:
 * 1000 spheres / 0 patches
 * 1280x720 @ 4spp
 * 4 threads
 * Intermediate image writing disabled
 * 2 potints
 * 23.5 second
 *
 * Threaded ray-reordering w/ counting sort and new ThreadQueue:
 * 1000 spheres / 0 patches
 * 1280x720 @ 4spp
 * 4 threads
 * Intermediate image writing disabled
 * 2 potints
 * 21.3 second
 */

#define THREADS 4
#define SPP 4
//#define XRES 512
//#define YRES 288
#define XRES 1280
#define YRES 720
#define NUM_RAND_PATCHES 1000
#define NUM_RAND_SPHERES 0000
#define SPHERE_RADIUS 1.0
#define FRAC_MB 0.1
#define CAMERA_SPIN 0.0
#define LENS_DIAM 1.0


// Holds a pair of integers as a resolution
// For use by boost program options
struct Resolution {
	int x;
	int y;

	Resolution() {
		x = 0;
		y = 0;
	}

	Resolution(int x_, int y_) {
		x = x_;
		y = y_;
	}
};

// Called by program_options to parse a set of Resolution arguments
void validate(boost::any& v, const std::vector<std::string>& values,
              Resolution*, int)
{
	Resolution res;

	//Extract tokens from values string vector and populate IntPair struct.
	if (values.size() < 2) {
		throw BPO::validation_error(BPO::validation_error::invalid_option_value,
		                            "Invalid Resolution specification, requires two ints");
	}

	res.x = boost::lexical_cast<int>(values.at(0));
	res.y = boost::lexical_cast<int>(values.at(1));

	v = res;
}


int main(int argc, char **argv)
{
	// Profiling
	ProfilerStart("psychopath.prof");

	// RNGs
	RNG rng(0);
	RNG rng2(1);

	/*
	 **********************************************************************
	 * Print program information
	 **********************************************************************
	 */
	std::cout << "Psychopath v" << VERSION_MAJOR << "." << VERSION_MINOR << "." << VERSION_PATCH;
#ifdef DEBUG
	std::cout << " (DEBUG build)";
#endif
	std::cout << std::endl << std::endl;

#ifdef DEBUG
	std::cout << std::endl << "Struct sizes:" << std::endl;
	std::cout << "\tvoid*: " << sizeof(void*) << std::endl;
	std::cout << "\tVec3: " << sizeof(Vec3) << std::endl;
	std::cout << "\tBBox: " << sizeof(BBox) << std::endl;
	std::cout << "\tRay: " << sizeof(Ray) << std::endl;
	std::cout << "\tIntersection: " << sizeof(Intersection) << std::endl;
	std::cout << "\tRayInter: " << sizeof(RayInter) << std::endl;
	std::cout << "\tPotentialInter: " << sizeof(PotentialInter) << std::endl;
	std::cout << "\tBVHNode: " << sizeof(BVHNode) << std::endl;
	std::cout << "\tMicroSurface: " << sizeof(MicroSurface) << std::endl;
	std::cout << "\tMicroNode: " << sizeof(MicroNode) << std::endl;
	std::cout << "\tTimeBox<int32>: " << sizeof(std::vector<int32>) << std::endl;
#endif


	/*
	 **********************************************************************
	 * Command-line options.
	 **********************************************************************
	 */
	int spp = SPP;
	int threads = THREADS;
	std::string output_path = "default.png";
	Resolution resolution(XRES, YRES);

	// Define them
	BPO::options_description desc("Allowed options");
	desc.add_options()
	("help,h", "Print this help message")
	("spp,s", BPO::value<int>(), "Number of samples to take per pixel")
	("threads,t", BPO::value<int>(), "Number of threads to render with")
	("output,o", BPO::value<std::string>(), "The PNG file to render to")
	("resolution,r", BPO::value<Resolution>()->multitoken(), "The resolution to render at, e.g. 1280 720")
	;

	// Collect them
	BPO::variables_map vm;
	BPO::store(BPO::parse_command_line(argc, argv, desc), vm);
	BPO::notify(vm);

	// Help message
	if (vm.count("help")) {
		std::cout << desc << "\n";
		return 1;
	}

	// Samples per pixel
	if (vm.count("spp")) {
		spp = vm["spp"].as<int>();
		if (spp < 1)
			spp = 1;
		std::cout << "Samples per pixel: " << spp << "\n";
	}

	// Thread count
	if (vm.count("threads")) {
		threads = vm["threads"].as<int>();
		if (threads < 1)
			threads = 1;
		std::cout << "Threads: " << threads << "\n";
	}

	// Output file
	if (vm.count("output")) {
		output_path = vm["output"].as<std::string>();
		std::cout << "Output path: " << output_path << "\n";
	}

	// Resolution
	if (vm.count("resolution")) {
		resolution = vm["resolution"].as<Resolution>();
		std::cout << "Resolution: " << resolution.x << " " << resolution.y << "\n";
	}


	/*
	 **********************************************************************
	 * Build scene
	 **********************************************************************
	 */
	Scene scene;

	// Add camera
	Matrix44 mat;
	std::vector<Transform> cam_tra;
	cam_tra.resize(4);

	float angle = CAMERA_SPIN * (3.14159 / 180.0);
	ImathVec3 axis(0.0, 0.0, 1.0);

	mat.makeIdentity();
	mat.translate(ImathVec3(0.0, 0.0, -40.0));
	mat.rotate(ImathVec3(0.0, 0.0, 0.0));
	mat.translate(ImathVec3(0.0, 0.0, 20.0));
	cam_tra[0] = mat;

	mat.makeIdentity();
	mat.translate(ImathVec3(0.0, 0.0, -40.0));
	mat.rotate(ImathVec3(0.0, 0.0, (angle/3)*1));
	mat.translate(ImathVec3(0.0, 0.0, 20.0));
	cam_tra[1] = mat;

	mat.makeIdentity();
	mat.translate(ImathVec3(0.0, 0.0, -40.0));
	mat.rotate(ImathVec3(0.0, 0.0, (angle/3)*2));
	mat.translate(ImathVec3(0.0, 0.0, 20.0));
	cam_tra[2] = mat;

	mat.makeIdentity();
	mat.translate(ImathVec3(0.0, 0.0, -40.0));
	mat.rotate(ImathVec3(0.0, 0.0, (angle/3)*3));
	mat.translate(ImathVec3(0.0, 0.0, 20.0));
	cam_tra[3] = mat;

#define FOCUS_DISTANCE 40.0
#define FOV 55
	scene.camera = new Camera(cam_tra, (3.14159/180.0)*FOV, LENS_DIAM, FOCUS_DISTANCE);


	// Add lights
	SphereLight *sl = new SphereLight(Vec3(10.0, 10.0, -10.0),
	                                  2.0,
	                                  Color(250.0));
	scene.add_finite_light(sl);
	//PointLight *pl = new PointLight(Vec3(-10.0, 0.0, -10.0),
	//                                Color(20.0));
	//scene.add_finite_light(pl);


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
		bool flip = true;
		//if (rng.next_float() < 0.5)
		//	flip = false;

		patch = new Bilinear(ms);

		for (int j = 0; j < ms; j++) {
			x += (rng.next_float_c() * j * 4) / s;
			y += (rng.next_float_c() * j * 4) / s;
			z += (rng.next_float_c() * j * 4) / s;

			if (flip) {
				patch->add_time_sample(j,
				                       Vec3((x*s)+2, (y*s)+2, z+(rng.next_float_c()*8)),
				                       Vec3((x*s)+2, (y*s)-2, z+(rng.next_float_c()*8)),
				                       Vec3((x*s)-2, (y*s)-2, z+(rng.next_float_c()*8)),
				                       Vec3((x*s)-2, (y*s)+2, z+(rng.next_float_c()*8)));
			} else {
				patch->add_time_sample(j,
				                       Vec3((x*s)+2, (y*s)+2, z+(rng.next_float_c()*8)),
				                       Vec3((x*s)-2, (y*s)+2, z+(rng.next_float_c()*8)),
				                       Vec3((x*s)-2, (y*s)-2, z+(rng.next_float_c()*8)),
				                       Vec3((x*s)+2, (y*s)-2, z+(rng.next_float_c()*8)));

			}
		}

//#define SPLIT_PATCH
#ifdef SPLIT_PATCH
		std::vector<Primitive *> splits1;
		std::vector<Primitive *> splits2;
		std::vector<Primitive *> splits3;

		patch->split(splits1);
		delete patch;

		patch = (Bilinear *)(splits1[0]);
		patch->split(splits2);
		delete patch;
		patch = (Bilinear *)(splits1[1]);
		patch->split(splits3);
		delete patch;

		scene.add_primitive(splits2[0]);
		scene.add_primitive(splits2[1]);
		scene.add_primitive(splits3[0]);
		scene.add_primitive(splits3[1]);
#else
		scene.add_primitive(patch);
#endif

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
		z = 15 + (rng2.next_float() * NUM_RAND_SPHERES / 4) * radius;
		float32 s = z / 15;
		x = rng2.next_float_c() * 40;
		y = rng2.next_float_c() * 20;

		// Motion?
		int ms = 1;
		if (rng2.next_float() < FRAC_MB)
			ms = 2;

		sphere = new Sphere(ms);

		for (int j = 0; j < ms; j++) {
			x += (rng2.next_float_c() * j * 4) / s;
			y += (rng2.next_float_c() * j * 4) / s;
			z += (rng2.next_float_c() * j * 4) / s;

			sphere->add_time_sample(j,
			                        Vec3(x*s, y*s, z+(rng2.next_float_c()*2)),
			                        SPHERE_RADIUS);
		}

		scene.add_primitive(sphere);
	}
	std::cout << " done." << std::endl;
	std::cout.flush();

	std::cout << "Finalizing scene... ";
	std::cout.flush();
	scene.finalize();
	std::cout << " done." << std::endl;
	std::cout.flush();


	/*
	 **********************************************************************
	 * Generate image
	 **********************************************************************
	 */

	std::cout << "\nStarting render: \n";
	std::cout.flush();
	Renderer r(&scene, resolution.x, resolution.y, spp, output_path);
	r.render(threads);

	ProfilerStop();
	return 0;
}
