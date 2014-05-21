#include "parser.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <tuple>
#include <memory>
#include <boost/regex.hpp>

#include "color.hpp"
#include "vector.hpp"
#include "matrix.hpp"
#include "transform.hpp"

#include "sphere_light.hpp"
#include "sphere.hpp"
#include "bilinear.hpp"
#include "bicubic.hpp"

#include "renderer.hpp"
#include "scene.hpp"


static boost::regex re_int("-?[0-9]+");
static boost::regex re_float("-?[0-9]+[.]?[0-9]*");

static boost::regex re_quote("\"");
static boost::regex re_qstring("\".*\"");


static bool getline(std::ifstream &f, std::string &l)
{
	const bool t = bool(std::getline(f, l));
	//std::cout << "\t" << l << std::endl;
	return t;
}

/**
 * @brief Backs up a single line in the file.
 */
static void ungetline(std::ifstream &f)
{
	if (f.tellg() != 0)
		f.unget();

	while (f.tellg() != 0) {
		f.unget();
		if (f.peek() == '\n') {
			f.get();
			break;
		}
	}
}



std::unique_ptr<Renderer> Parser::parse_next_frame()
{
	std::string line;

	int res_x = 0, res_y = 0;
	int spp = 0;
	int seed = 0;
	std::string output_path("");

	// Get frame
	while (true) {
		if (!getline(psy_file, line)) {
			return nullptr;
		}

		if (line.find("Frame") == 0) { // Check if the line starts with "Frame"
			std::tie(res_x, res_y, spp, seed, output_path) = parse_frame_header();
			std::cout << "Frame:\n  \"" << output_path << "\"\n  " << res_x << "x" << res_y << "\n  spp: " << spp << "\n  seed: " << seed << " " << std::endl;
			break;
		}
	}

	// Create scene, to populate
	std::unique_ptr<Scene> scene {new Scene()};

	// Get camera
	while (true) {
		if (!getline(psy_file, line)) {
			std::cout << "No camera section.  Can't render." << std::endl;
			return nullptr;
		}

		if (line == "Camera") { // Check if the line starts with "Camera"
			//std::cout << "Found Camera" << std::endl;
			scene->camera = parse_camera();
			break;
		} else if (line == "Frame") {
			ungetline(psy_file);
			std::cout << "No camera section.  Can't render." << std::endl;
			return nullptr;
		}
	}

	const std::string bilinear_patch_header {"BilinearPatch "};
	const std::string bicubic_patch_header {"BicubicPatch "
	};
	const std::string sphere_header {"Sphere "
	};
	const std::string sphere_light_header {"SphereLight "
	};

	// Populate scene
	while (true) {
		if (!getline(psy_file, line))
			break;

		if (line.find(bilinear_patch_header) == 0) {
			// Parse a bilinear patch
			std::string name = line.substr(bilinear_patch_header.length());

			//std::cout << "Found BilinearPatch: " << name << std::endl;
			scene->root->add_object(name, parse_bilinear_patch());
		} else if (line.find(bicubic_patch_header) == 0) {
			// Parse a bicubic patch
			std::string name = line.substr(bicubic_patch_header.length());

			//std::cout << "Found BicubicPatch: " << name << std::endl;
			scene->root->add_object(name, parse_bicubic_patch());
		} else if (line.find(sphere_header) == 0) {
			// Parse a sphere
			std::string name = line.substr(sphere_header.length());

			//std::cout << "Found Sphere: " << name << std::endl;
			scene->root->add_object(name, parse_sphere());
		} else if (line.find(sphere_light_header) == 0) {
			// Parse a spherical light
			std::string name = line.substr(sphere_light_header.length());

			//std::cout << "Found SphereLight: " << name << std::endl;
			scene->root->add_object(name, parse_sphere_light());
		} else if (line == "Frame") {
			ungetline(psy_file);
			break;
		}
	}

	scene->finalize();

	std::unique_ptr<Renderer> renderer(new Renderer(scene.release(), res_x, res_y, spp, spp, 0.0f, seed, output_path));

	return renderer;
}


std::tuple<int, int, int, int, std::string> Parser::parse_frame_header()
{
	int res_x = 0, res_y = 0;
	int spp = 0;
	int seed = 0;
	std::string output_path("");

	std::string line;

	// Loop through the lines
	while (getline(psy_file, line)) {
		if (line.find("Resolution:") == 0) {
			// Get the output resolution
			boost::sregex_iterator matches(line.begin(), line.end(), re_int);
			for (int i = 0; matches != boost::sregex_iterator() && i < 2; ++matches) {
				if (i == 0)
					res_x = std::stoi(matches->str());
				else
					res_y = std::stoi(matches->str());
				++i;
			}
		} else if (line.find("Samples:") == 0) {
			// Get the number of samples per pixel
			boost::sregex_iterator matches(line.begin(), line.end(), re_int);
			if (matches != boost::sregex_iterator()) {
				spp = std::stoi(matches->str());
			}
		} else if (line.find("Seed:") == 0) {
			// Get the number of samples per pixel
			boost::sregex_iterator matches(line.begin(), line.end(), re_int);
			if (matches != boost::sregex_iterator()) {
				seed = std::stoi(matches->str());
			}
		} else if (line.find("Output:") == 0) {
			// Get the output path
			boost::sregex_iterator matches(line.begin(), line.end(), re_qstring);
			if (matches != boost::sregex_iterator()) {
				output_path = boost::regex_replace(matches->str(), re_quote, "");
			}
		} else {
			// Not a valid line for this section, stop
			ungetline(psy_file);
			break;
		}
	}

	return std::tuple<int, int, int, int, std::string>(res_x, res_y, spp, seed, output_path);
}


std::unique_ptr<Camera> Parser::parse_camera()
{
	std::vector<Matrix44> mats;
	float fov = 90.0f;
	float focus_distance = 10.0f;
	float aperture_size = 0.0f;

	std::string line;

	// Loop through the lines
	while (getline(psy_file, line)) {
		if (line.find("Matrix:") == 0) {
			// Get the camera matrix(s); multiple matrices means motion blur
			ungetline(psy_file);
			while (getline(psy_file, line)) {
				if (line.find("Matrix:") == 0) {
					float matvals[16] {1, 0, 0, 0,  0, 1, 0, 0,  0, 0, 1, 0,  0, 0, 0, 1};
					boost::sregex_iterator matches(line.begin(), line.end(), re_float);
					for (int i = 0; matches != boost::sregex_iterator() && i < 16; ++matches) {
						matvals[i] = std::stof(matches->str());
						++i;
					}

					Matrix44 mat;
					for (int i = 0; i < 4; ++i) {
						for (int j = 0; j < 4; ++j) {
							mat[i][j] = matvals[i*4 + j];
						}
					}
					mats.push_back(mat);
				} else {
					// No more verts
					ungetline(psy_file);
					break;
				}
			}
		} else if (line.find("FOV:") == 0) {
			// Get the camera's field of view
			boost::sregex_iterator matches(line.begin(), line.end(), re_float);
			if (matches != boost::sregex_iterator()) {
				fov = std::stof(matches->str());
			}
		} else if (line.find("FocusDistance:") == 0) {
			// Get the camera's field of view
			boost::sregex_iterator matches(line.begin(), line.end(), re_float);
			if (matches != boost::sregex_iterator()) {
				focus_distance = std::stof(matches->str());
			}
		} else if (line.find("ApertureSize:") == 0) {
			// Get the camera's field of view
			boost::sregex_iterator matches(line.begin(), line.end(), re_float);
			if (matches != boost::sregex_iterator()) {
				aperture_size = std::stof(matches->str());
			}
		} else {
			// Not a valid line for this section, stop
			ungetline(psy_file);
			break;
		}
	}

	// Build camera transforms
	std::vector<Transform> cam_transforms;
	cam_transforms.resize(mats.size());
	for (uint i = 0; i < mats.size(); ++i)
		cam_transforms[i] = mats[i];

	// Construct camera
	std::unique_ptr<Camera> camera(new Camera(cam_transforms, (3.14159/180.0)*fov, aperture_size, focus_distance));

	return camera;
}


std::unique_ptr<SphereLight> Parser::parse_sphere_light()
{
	Vec3 location {0,0,0};
	Color color {0,0,0};
	float energy {1.0f};
	float radius {0.5f};

	std::string line;

	// Loop through the lines
	while (getline(psy_file, line)) {
		if (line.find("Location:") == 0) {
			// Get the camera's location
			boost::sregex_iterator matches(line.begin(), line.end(), re_float);
			for (int i = 0; matches != boost::sregex_iterator() && i < 3; ++matches) {
				location[i] = std::stof(matches->str());
				++i;
			}
		} else if (line.find("Color:") == 0) {
			// Get the camera's rotation
			boost::sregex_iterator matches(line.begin(), line.end(), re_float);
			for (int i = 0; matches != boost::sregex_iterator() && i < 3; ++matches) {
				color[i] = std::stof(matches->str());
				++i;
			}
		} else if (line.find("Energy:") == 0) {
			// Get the camera's field of view
			boost::sregex_iterator matches(line.begin(), line.end(), re_float);
			if (matches != boost::sregex_iterator()) {
				energy = std::stof(matches->str());
			}
		} else if (line.find("Radius:") == 0) {
			// Get the camera's field of view
			boost::sregex_iterator matches(line.begin(), line.end(), re_float);
			if (matches != boost::sregex_iterator()) {
				radius = std::stof(matches->str());
			}
		} else {
			// Not a valid line for this section, stop
			ungetline(psy_file);
			break;
		}
	}

	// Build light
	std::unique_ptr<SphereLight> sl(new SphereLight(location,
	                                radius,
	                                color * energy));

	return sl;
}


struct BilinearPatchVerts {
	float v[12];
};

std::unique_ptr<Bilinear> Parser::parse_bilinear_patch()
{
	std::vector<BilinearPatchVerts> patch_verts;

	std::string line;

	// Get the vertices of the patch; multiple vert lines means motion blur
	while (getline(psy_file, line)) {
		if (line.find("Vertices:") == 0) {
			BilinearPatchVerts verts;
			boost::sregex_iterator matches(line.begin(), line.end(), re_float);
			for (int i = 0; matches != boost::sregex_iterator() && i < 12; ++matches) {
				verts.v[i] = std::stof(matches->str());
				++i;
			}
			patch_verts.push_back(verts);
		} else {
			// No more verts
			ungetline(psy_file);
			break;
		}
	}

	// Build the patch
	std::unique_ptr<Bilinear> patch(new Bilinear());
	for (uint i = 0; i < patch_verts.size(); ++i) {
		auto p = patch_verts[i];
		patch->add_time_sample(Vec3(p.v[0], p.v[1], p.v[2]),
		                       Vec3(p.v[3], p.v[4], p.v[5]),
		                       Vec3(p.v[6], p.v[7], p.v[8]),
		                       Vec3(p.v[9], p.v[10], p.v[11]));
	}

	patch->finalize();

	return patch;
}


struct BicubicPatchVerts {
	float v[48];
};

std::unique_ptr<Bicubic> Parser::parse_bicubic_patch()
{
	std::vector<BicubicPatchVerts> patch_verts;

	std::string line;

	// Get the vertices of the patch; multiple vert lines means motion blur
	while (getline(psy_file, line)) {
		if (line.find("Vertices:") == 0) {
			BicubicPatchVerts verts;
			boost::sregex_iterator matches(line.begin(), line.end(), re_float);
			for (int i = 0; matches != boost::sregex_iterator() && i < 48; ++matches) {
				verts.v[i] = std::stof(matches->str());
				++i;
			}
			patch_verts.push_back(verts);
		} else {
			// No more verts
			ungetline(psy_file);
			break;
		}
	}

	// Build the patch
	std::unique_ptr<Bicubic> patch(new Bicubic());
	for (auto& p: patch_verts) {
		patch->add_time_sample(Vec3(p.v[0], p.v[1], p.v[2]),
		                       Vec3(p.v[3], p.v[4], p.v[5]),
		                       Vec3(p.v[6], p.v[7], p.v[8]),
		                       Vec3(p.v[9], p.v[10], p.v[11]),

		                       Vec3(p.v[12], p.v[13], p.v[14]),
		                       Vec3(p.v[15], p.v[16], p.v[17]),
		                       Vec3(p.v[18], p.v[19], p.v[20]),
		                       Vec3(p.v[21], p.v[22], p.v[23]),

		                       Vec3(p.v[24], p.v[25], p.v[26]),
		                       Vec3(p.v[27], p.v[28], p.v[29]),
		                       Vec3(p.v[30], p.v[31], p.v[32]),
		                       Vec3(p.v[33], p.v[34], p.v[35]),

		                       Vec3(p.v[36], p.v[37], p.v[38]),
		                       Vec3(p.v[39], p.v[40], p.v[41]),
		                       Vec3(p.v[42], p.v[43], p.v[44]),
		                       Vec3(p.v[45], p.v[46], p.v[47]));
	}

	patch->finalize();

	return patch;
}


std::unique_ptr<Sphere> Parser::parse_sphere()
{
	// TODO: motion blur for spheres
	Vec3 location {0,0,0};
	float radius {0.5f};

	std::string line;

	// Loop through the lines
	while (getline(psy_file, line)) {
		if (line.find("Location:") == 0) {
			// Get the camera's location
			boost::sregex_iterator matches(line.begin(), line.end(), re_float);
			for (int i = 0; matches != boost::sregex_iterator() && i < 3; ++matches) {
				location[i] = std::stof(matches->str());
				++i;
			}
		} else if (line.find("Radius:") == 0) {
			// Get the camera's field of view
			boost::sregex_iterator matches(line.begin(), line.end(), re_float);
			if (matches != boost::sregex_iterator()) {
				radius = std::stof(matches->str());
			}
		} else {
			// Not a valid line for this section, stop
			ungetline(psy_file);
			break;
		}
	}

	// Build light
	std::unique_ptr<Sphere> s(new Sphere(location, radius));

	return s;
}

