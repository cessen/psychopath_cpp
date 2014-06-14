#include "parser.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <tuple>
#include <memory>
#include <boost/regex.hpp>

#include "config.hpp"

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




std::unique_ptr<Renderer> Parser::parse_next_frame()
{
	unsigned int scene_node_index;

	// Find next scene node, or return nullptr if no more are found
	while (true) {
		if (tree.children.size() < (node_index + 1))
			return nullptr;

		if (tree.children[node_index].type == "Scene") {
			scene_node_index = node_index;
			node_index++;
			break;
		}

		node_index++;
	}

	// Create scene, to populate
	std::unique_ptr<Scene> scene {new Scene()};

	// Set scene name
	scene->name = tree.children[scene_node_index].name.substr(1);

	// Parse scene
	int res_x = 1, res_y = 1;
	int spp = 1;
	int seed = 1;
	std::string output_path("");

	for (const auto& node: tree.children[scene_node_index].children) {
		// Output section
		if (node.type == "Output") {
			for (const auto& child: node.children) {
				if (child.type == "Path") {
					// Get the output path
					boost::sregex_iterator matches(child.leaf_contents.begin(), child.leaf_contents.end(), re_qstring);
					if (matches != boost::sregex_iterator()) {
						output_path = boost::regex_replace(matches->str(), re_quote, "");
					}
				} else if (child.type == "Format") {
					// TODO
				} else if (child.type == "ColorSpace") {
					// TODO
				} else if (child.type == "Dither") {
					// TODO
				}
			}
		}

		// Render settings section
		else if (node.type == "RenderSettings") {
			for (const auto& child: node.children) {
				if (child.type == "Resolution") {
					// Get the output resolution
					boost::sregex_iterator matches(child.leaf_contents.begin(), child.leaf_contents.end(), re_int);
					for (int i = 0; matches != boost::sregex_iterator() && i < 2; ++matches) {
						if (i == 0)
							res_x = std::stoi(matches->str());
						else
							res_y = std::stoi(matches->str());
						++i;
					}
				} else if (child.type == "PixelAspect") {
					// TODO
				} else if (child.type == "SamplesPerPixel") {
					// Get the number of samples per pixel
					boost::sregex_iterator matches(child.leaf_contents.begin(), child.leaf_contents.end(), re_int);
					if (matches != boost::sregex_iterator()) {
						spp = std::stoi(matches->str());
					}
				} else if (child.type == "Filter") {
					// TODO
				} else if (child.type == "DicingRate") {
					// Get aperture size
					boost::sregex_iterator matches(child.leaf_contents.begin(), child.leaf_contents.end(), re_float);
					if (matches != boost::sregex_iterator()) {
						Config::dice_rate = std::stof(matches->str());
					}
				} else if (child.type == "Seed") {
					// Get the seed for the frame
					boost::sregex_iterator matches(child.leaf_contents.begin(), child.leaf_contents.end(), re_int);
					if (matches != boost::sregex_iterator()) {
						seed = std::stoi(matches->str());
					}
				}
			}
		}

		// Camera description
		else if (node.type == "Camera") {
			scene->camera = parse_camera(node);
		}

		// World description
		else if (node.type == "World") {
			// TODO
		}

		// Root Assembly definition
		else if (node.type == "Assembly") {
			scene->root = parse_assembly(node);
		}
	}



	std::cout << "Scene: " << scene->name << std::endl;
	std::cout << "Output Path: " << output_path << std::endl;
	std::cout << "Resolution: " << res_x << "x" << res_y << std::endl;
	std::cout << "SPP: " << spp << std::endl;
	std::cout << "Seed: " << seed << std::endl;

	// return nullptr;

	scene->finalize();

	std::unique_ptr<Renderer> renderer(new Renderer(scene.release(), res_x, res_y, spp, spp, 0.0f, seed, output_path));

	return renderer;
}


Matrix44 Parser::parse_matrix(const std::string line)
{
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

	return mat;
}


std::unique_ptr<Camera> Parser::parse_camera(const DataTree::Node& node)
{
	std::vector<Matrix44> mats;
	std::vector<float> fovs;
	std::vector<float> focus_distances;
	std::vector<float> aperture_radii;

	// Parse
	for (const auto& child: node.children) {
		if (child.type == "Fov") {
			// Get FOV
			boost::sregex_iterator matches(child.leaf_contents.begin(), child.leaf_contents.end(), re_float);
			if (matches != boost::sregex_iterator()) {
				fovs.emplace_back((3.1415926536f/180.0f) * std::stof(matches->str()));
			}
		} else if (child.type == "FocalDistance") {
			// Get focal distance
			boost::sregex_iterator matches(child.leaf_contents.begin(), child.leaf_contents.end(), re_float);
			if (matches != boost::sregex_iterator()) {
				focus_distances.emplace_back(std::stof(matches->str()));
			}
		} else if (child.type == "ApertureRadius") {
			// Get aperture size
			boost::sregex_iterator matches(child.leaf_contents.begin(), child.leaf_contents.end(), re_float);
			if (matches != boost::sregex_iterator()) {
				aperture_radii.emplace_back(std::stof(matches->str()));
			}
		} else if (child.type == "Transform") {
			// Get transform matrix
			mats.emplace_back(parse_matrix(child.leaf_contents));
		}
	}

	// Build camera transforms
	std::vector<Transform> cam_transforms;
	cam_transforms.resize(mats.size());
	for (uint i = 0; i < mats.size(); ++i)
		cam_transforms[i] = mats[i];

	// Construct camera
	std::unique_ptr<Camera> camera(new Camera(cam_transforms, fovs, aperture_radii, focus_distances));

	return camera;
}


std::unique_ptr<Assembly> Parser::parse_assembly(const DataTree::Node& node)
{
	// Allocate assembly
	std::unique_ptr<Assembly> assembly = std::unique_ptr<Assembly>(new Assembly());

	for (const auto& child: node.children) {
		// Sub-Assembly
		if (child.type == "Assembly") {
			assembly->add_assembly(child.name, parse_assembly(child));
		}

		// Bilinear Patch
		else if (child.type == "BilinearPatch") {
			assembly->add_object(child.name, parse_bilinear_patch(child));
		}

		// Bicubic Patch
		else if (child.type == "BicubicPatch") {
			assembly->add_object(child.name, parse_bicubic_patch(child));
		}

		// Spehere Light
		else if (child.type == "Sphere") {
			assembly->add_object(child.name, parse_sphere(child));
		}

		// Spehere Light
		else if (child.type == "SphereLight") {
			assembly->add_object(child.name, parse_sphere_light(child));
		}

		// Instance
		else if (child.type == "Instance") {
			// Parse
			std::string name = "";
			std::vector<Transform> xforms;
			for (const auto& child2: child.children) {
				if (child2.type == "Transform") {
					xforms.emplace_back(parse_matrix(child2.leaf_contents));
				} else if (child2.type == "Data") {
					name = child2.leaf_contents;
				}
			}

			// Add instance
			if (assembly->object_map.count(name) != 0) {
				assembly->create_object_instance(name, xforms);
			} else if (assembly->assembly_map.count(name) != 0) {
				assembly->create_assembly_instance(name, xforms);
			} else {
				std::cout << "ERROR: attempted to add instace for data that doesn't exist." << std::endl;
			}
		}
	}

	assembly->optimize();

	return assembly;
}


std::unique_ptr<Bilinear> Parser::parse_bilinear_patch(const DataTree::Node& node)
{
	struct BilinearPatchVerts {
		float v[12];
	};

	std::vector<BilinearPatchVerts> patch_verts;

	for (const auto& child: node.children) {
		// Vertex list
		if (child.type == "Vertices") {
			BilinearPatchVerts verts;
			boost::sregex_iterator matches(child.leaf_contents.begin(), child.leaf_contents.end(), re_float);
			for (int i = 0; matches != boost::sregex_iterator() && i < 12; ++matches) {
				verts.v[i] = std::stof(matches->str());
				++i;
			}
			patch_verts.push_back(verts);
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


std::unique_ptr<Bicubic> Parser::parse_bicubic_patch(const DataTree::Node& node)
{
	struct BicubicPatchVerts {
		float v[48];
	};

	std::vector<BicubicPatchVerts> patch_verts;

	for (const auto& child: node.children) {
		// Vertex list
		if (child.type == "Vertices") {
			BicubicPatchVerts verts;
			boost::sregex_iterator matches(child.leaf_contents.begin(), child.leaf_contents.end(), re_float);
			for (int i = 0; matches != boost::sregex_iterator() && i < 48; ++matches) {
				verts.v[i] = std::stof(matches->str());
				++i;
			}
			patch_verts.push_back(verts);
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


std::unique_ptr<SphereLight> Parser::parse_sphere_light(const DataTree::Node& node)
{
	std::vector<Color> colors;
	std::vector<Vec3> locations;
	std::vector<float> radii;

	// Parse
	for (const auto& child: node.children) {
		if (child.type == "Color") {
			// Get color
			boost::sregex_iterator matches(child.leaf_contents.begin(), child.leaf_contents.end(), re_float);
			Color col;
			for (int i = 0; matches != boost::sregex_iterator() && i < 3; ++matches) {
				col[i] = std::stof(matches->str());
				++i;
			}
			colors.push_back(col);
		} else if (child.type == "Radius") {
			// Get radius
			boost::sregex_iterator matches(child.leaf_contents.begin(), child.leaf_contents.end(), re_float);
			if (matches != boost::sregex_iterator()) {
				radii.emplace_back(std::stof(matches->str()));
			}
		} else if (child.type == "Location") {
			// Get location
			boost::sregex_iterator matches(child.leaf_contents.begin(), child.leaf_contents.end(), re_float);
			Vec3 loc;
			for (int i = 0; matches != boost::sregex_iterator() && i < 3; ++matches) {
				loc[i] = std::stof(matches->str());
				++i;
			}
			locations.push_back(loc);
		}
	}

	// Build light
	std::unique_ptr<SphereLight> sl(new SphereLight(locations,
	                                radii,
	                                colors));

	return sl;
}


std::unique_ptr<Sphere> Parser::parse_sphere(const DataTree::Node& node)
{
	// TODO: motion blur for spheres
	Vec3 location {0,0,0};
	float radius {0.5f};

	// Parse
	for (const auto& child: node.children) {
		if (child.type == "Radius") {
			// Get radius
			boost::sregex_iterator matches(child.leaf_contents.begin(), child.leaf_contents.end(), re_float);
			if (matches != boost::sregex_iterator()) {
				radius = std::stof(matches->str());
			}
		} else if (child.type == "Location") {
			// Get location
			boost::sregex_iterator matches(child.leaf_contents.begin(), child.leaf_contents.end(), re_float);
			for (int i = 0; matches != boost::sregex_iterator() && i < 3; ++matches) {
				location[i] = std::stof(matches->str());
				++i;
			}
		}
	}

	// Build sphere
	std::unique_ptr<Sphere> s(new Sphere(location, radius));

	return s;
}
