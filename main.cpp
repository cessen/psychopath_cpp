#include "config.h"
#include "numtype.h"

#include <cstdlib>
#include <cmath>

#include <thread>
#include <iostream>
#include <vector>

//#include <OSL/oslexec.h>
#include <boost/program_options.hpp>

#include "rng.hpp"
#include "renderer.hpp"
#include "scene.hpp"
#include "vector.hpp"
#include "matrix.hpp"
#include "transform.hpp"
#include "camera.hpp"

#include "light.hpp"
#include "point_light.hpp"
#include "sphere_light.hpp"

#include "ray.hpp"
#include "intersection.hpp"
#include "potentialinter.hpp"

#include "global.hpp"

#include "timer.hpp"

#include "parser.hpp"
#include "data_tree.hpp"

namespace BPO = boost::program_options;


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

// Holds four integers that specify a rectangle within the image.
// For use by boost program options
struct SubImage {
	int x1, y1;
	int x2, y2;

	SubImage() {
		x1 = 0;
		y1 = 0;
		x2 = 0;
		y2 = 0;
	}

	SubImage(int x1_, int y1_, int x2_, int y2_) {
		x1 = x1_;
		y1 = y1_;
		x2 = x2_;
		y2 = y2_;
	}
};

// Called by program_options to parse a set of Resolution arguments
void validate(boost::any& v, const std::vector<std::string>& values,
              Resolution*, int)
{
	Resolution res;

	//Extract tokens from values string vector and populate Resolution struct.
	if (values.size() < 2) {
		throw BPO::validation_error(BPO::validation_error::invalid_option_value,
		                            "Invalid Resolution specification, requires two ints");
	}

	res.x = boost::lexical_cast<int>(values.at(0));
	res.y = boost::lexical_cast<int>(values.at(1));

	v = res;
}

// Called by program_options to parse a set of SubImage arguments
void validate(boost::any& v, const std::vector<std::string>& values,
              SubImage*, int)
{
	SubImage subim;

	//Extract tokens from values string vector and populate SubImage struct.
	if (values.size() < 4) {
		throw BPO::validation_error(BPO::validation_error::invalid_option_value,
		                            "Invalid SubImage specification, requires four ints");
	}

	subim.x1 = boost::lexical_cast<int>(values.at(0));
	subim.y1 = boost::lexical_cast<int>(values.at(1));
	subim.x2 = boost::lexical_cast<int>(values.at(2));
	subim.y2 = boost::lexical_cast<int>(values.at(3));

	v = subim;
}


int main(int argc, char **argv)
{
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
	std::cout << std::endl;

#ifdef DEBUG
	std::cout << std::endl << "Struct sizes:" << std::endl;
	std::cout << "\tvoid*: " << sizeof(void*) << std::endl;
	std::cout << "\tVec3: " << sizeof(Vec3) << std::endl;
	std::cout << "\tBBox: " << sizeof(BBox) << std::endl;
	std::cout << "\tRay: " << sizeof(Ray) << std::endl;
	std::cout << "\tIntersection: " << sizeof(Intersection) << std::endl;
	std::cout << "\tPotentialInter: " << sizeof(PotentialInter) << std::endl;
	std::cout << "\tBVH::Node: " << sizeof(BVH::Node) << std::endl;
#endif


	/*
	 **********************************************************************
	 * Command-line options.
	 **********************************************************************
	 */
	int spp = SPP;
	int spp_max = SPP;
	float variance_max = -1.0f;
	int threads = std::thread::hardware_concurrency();
	std::string output_path = "default.png";
	std::string input_path = "";
	Resolution resolution(XRES, YRES);
	SubImage subimage;

	// Define them
	BPO::options_description desc("Allowed options");
	desc.add_options()
	("help,h", "Print this help message")
	("scenefile,i", BPO::value<std::string>(), "Input scene file")
	("spp,s", BPO::value<int>(), "Number of samples to take per pixel")
	("sppmax,m", BPO::value<int>(), "Max number of samples to take per pixel")
	("variance,v", BPO::value<float>(), "Max image variance")
	("threads,t", BPO::value<int>(), "Number of threads to render with")
	("output,o", BPO::value<std::string>(), "The PNG file to render to")
	("nooutput,n", "Don't save render (for timing tests)")
	("resolution,r", BPO::value<Resolution>()->multitoken(), "The resolution to render at, e.g. 1280 720")
	("subimage", BPO::value<SubImage>()->multitoken(), "The portion of the image to render as x1 y1 x2 y2, e.g. 24 24 100 120")
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

	// Suppress image writing
	Config::no_output = bool(vm.count("nooutput"));

	// Samples per pixel
	if (vm.count("spp")) {
		spp = vm["spp"].as<int>();
		if (spp < 1)
			spp = 1;
		std::cout << "Samples per pixel: " << spp << "\n";
	}
	if (vm.count("sppmax")) {
		spp_max = vm["sppmax"].as<int>();
		if (spp_max < spp)
			spp_max = spp;
		std::cout << "Max samples per pixel: " << spp_max << "\n";
	}
	if (vm.count("variance")) {
		variance_max = vm["variance"].as<float>();
		std::cout << "Max image variance: " << variance_max << "\n";
	}

	// Thread count
	if (vm.count("threads")) {
		threads = vm["threads"].as<int>();
		if (threads < 1)
			threads = 1;
		std::cout << "Threads: " << threads << "\n";
	}

	// Input file
	if (vm.count("scenefile")) {
		input_path = vm["scenefile"].as<std::string>();
		std::cout << "Input scene: " << input_path << "\n";
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

	// Sub-image
	if (vm.count("subimage")) {
		subimage = vm["subimage"].as<SubImage>();
		std::cout << "SubImage: " << subimage.x1 << " " << subimage.y1 << " " << subimage.x2 << " " << subimage.y2 << "\n";
	}

	std::cout << std::endl;

	/*
	 ***********************************************
	 * Parse scene file, rendering frames as we go.
	 ***********************************************
	 */
	Parser parser(input_path);
	Timer<> total_timer;
	while (true) {
		Timer<> parse_timer;
		std::unique_ptr<Renderer> r {parser.parse_next_frame()};

		if (r == nullptr)
			break;

		std::cout << "Parse time (seconds): " << parse_timer.time() << std::endl;

		Timer<> preprocessing_timer;
		r->scene->finalize();
		std::cout << "Preprocessing time (seconds): " << preprocessing_timer.time() << std::endl;

		// Resolution and sampling overrides
		if (vm.count("resolution"))
			r->set_resolution(resolution.x, resolution.y);
		if (vm.count("subimage"))
			r->set_subimage(subimage.x1, subimage.y1, subimage.x1 + subimage.x2, subimage.y1 + subimage.y2);
		if (vm.count("spp"))
			r->set_spp(spp);
		if (vm.count("sppmax"))
			r->set_spp_max(spp_max);
		if (vm.count("variance"))
			r->set_variance_max(variance_max);

		/*
		 **********************************************************************
		 * Generate image
		 **********************************************************************
		 */
		r->render(threads);

		std::cout << std::endl << std::endl;
	}

	std::cout << "Total time (seconds): " << std::fixed << std::setprecision(3) << total_timer.time() << std::endl << std::endl;

	return 0;
}
