#include "parser.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <tuple>
#include <utility>
#include <memory>
#include <regex>

#include "config.hpp"

#include "color.hpp"
#include "vector.hpp"
#include "matrix.hpp"
#include "transform.hpp"

#include "sphere_light.hpp"
#include "rectangle_light.hpp"
#include "sphere.hpp"
#include "bilinear.hpp"
#include "bicubic.hpp"

#include "renderer.hpp"
#include "scene.hpp"


static std::regex re_int("-?[0-9]+");
static std::regex re_float("-?[0-9]+[.]?[0-9]*");

static std::regex re_quote("\"");
static std::regex re_qstring("\".*\"");




std::unique_ptr<Renderer> Parser::parse_next_frame() {
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
					std::sregex_iterator matches(child.leaf_contents.begin(), child.leaf_contents.end(), re_qstring);
					if (matches != std::sregex_iterator()) {
						output_path = std::regex_replace(matches->str(), re_quote, "");
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
					std::sregex_iterator matches(child.leaf_contents.begin(), child.leaf_contents.end(), re_int);
					for (int i = 0; matches != std::sregex_iterator() && i < 2; ++matches) {
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
					std::sregex_iterator matches(child.leaf_contents.begin(), child.leaf_contents.end(), re_int);
					if (matches != std::sregex_iterator()) {
						spp = std::stoi(matches->str());
					}
				} else if (child.type == "Filter") {
					// TODO
				} else if (child.type == "DicingRate") {
					// Get aperture size
					std::sregex_iterator matches(child.leaf_contents.begin(), child.leaf_contents.end(), re_float);
					if (matches != std::sregex_iterator()) {
						Config::dice_rate = std::stof(matches->str());
					}
				} else if (child.type == "Seed") {
					// Get the seed for the frame
					std::sregex_iterator matches(child.leaf_contents.begin(), child.leaf_contents.end(), re_int);
					if (matches != std::sregex_iterator()) {
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
			for (const auto& child: node.children) {
				// Parse background shader
				if (child.type == "BackgroundShader") {
					// Find the shader type
					auto shader_type = std::find_if(child.children.cbegin(), child.children.cend(), [](const DataTree::Node& child2) {
						return child2.type == "Type";
					});
					if (shader_type == child.children.cend()) {
						std::cout << "ERROR: attempted to add background shader without a type." << std::endl;
						return nullptr;
					}

					if (shader_type->leaf_contents == "Color") {
						Color col(0.0, 0.0, 0.0);
						for (const auto &child2: child.children) {
							if (child2.type == "Color") {
								// Get color
								std::sregex_iterator matches(child2.leaf_contents.begin(), child2.leaf_contents.end(), re_float);
								for (int i = 0; matches != std::sregex_iterator() && i < 3; ++matches) {
									col[i] = std::stof(matches->str());
									++i;
								}
							}
						}

						scene->background_color = col;
					}
				}

				// TODO: distant light sources
			}
		}

		// Root Assembly definition
		else if (node.type == "Assembly") {
			scene->root = parse_assembly(node, nullptr);
		}
	}



	std::cout << "Scene: " << scene->name << std::endl;
	std::cout << "Output Path: " << output_path << std::endl;
	std::cout << "Resolution: " << res_x << "x" << res_y << std::endl;
	std::cout << "SPP: " << spp << std::endl;
	std::cout << "Seed: " << seed << std::endl;

	// return nullptr;

	std::unique_ptr<Renderer> renderer(new Renderer(scene.release(), res_x, res_y, spp, spp, 0.0f, seed, output_path));

	return renderer;
}


Matrix44 Parser::parse_matrix(const std::string line) {
	float matvals[16] {1, 0, 0, 0,  0, 1, 0, 0,  0, 0, 1, 0,  0, 0, 0, 1};
	std::sregex_iterator matches(line.begin(), line.end(), re_float);
	for (int i = 0; matches != std::sregex_iterator() && i < 16; ++matches) {
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


std::unique_ptr<Camera> Parser::parse_camera(const DataTree::Node& node) {
	std::vector<Matrix44> mats;
	std::vector<float> fovs;
	std::vector<float> focus_distances;
	std::vector<float> aperture_radii;

	// Parse
	for (const auto& child: node.children) {
		if (child.type == "Fov") {
			// Get FOV
			std::sregex_iterator matches(child.leaf_contents.begin(), child.leaf_contents.end(), re_float);
			if (matches != std::sregex_iterator()) {
				fovs.emplace_back((3.1415926536f/180.0f) * std::stof(matches->str()));
			}
		} else if (child.type == "FocalDistance") {
			// Get focal distance
			std::sregex_iterator matches(child.leaf_contents.begin(), child.leaf_contents.end(), re_float);
			if (matches != std::sregex_iterator()) {
				focus_distances.emplace_back(std::stof(matches->str()));
			}
		} else if (child.type == "ApertureRadius") {
			// Get aperture size
			std::sregex_iterator matches(child.leaf_contents.begin(), child.leaf_contents.end(), re_float);
			if (matches != std::sregex_iterator()) {
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


std::unique_ptr<Assembly> Parser::parse_assembly(const DataTree::Node& node, const Assembly* parent_assembly) {
	// Allocate assembly
	std::unique_ptr<Assembly> assembly = std::unique_ptr<Assembly>(new Assembly());

	assembly->parent = parent_assembly;

	for (const auto& child: node.children) {
		// Sub-Assembly
		if (child.type == "Assembly") {
			assembly->add_assembly(child.name, parse_assembly(child, assembly.get()));
		}

		// Bilinear Patch
		else if (child.type == "BilinearPatch") {
			assembly->add_object(child.name, parse_bilinear_patch(child));
		}

		// Bicubic Patch
		else if (child.type == "BicubicPatch") {
			assembly->add_object(child.name, parse_bicubic_patch(child));
		}

		// Subdivision surface
		else if (child.type == "SubdivisionSurface") {
			assembly->add_object(child.name, parse_subdivision_surface(child));
		}

		// Sphere
		else if (child.type == "Sphere") {
			assembly->add_object(child.name, parse_sphere(child));
		}

		// Surface shader
		else if (child.type == "SurfaceShader") {
			assembly->add_surface_shader(child.name, parse_surface_shader(child));
		}

		// Sphere Light
		else if (child.type == "SphereLight") {
			assembly->add_object(child.name, parse_sphere_light(child));
		}

		// Rectangle Light
		else if (child.type == "RectangleLight") {
			assembly->add_object(child.name, parse_rectangle_light(child));
		}

		// Instance
		else if (child.type == "Instance") {
			// Parse
			std::string name = "";
			std::vector<Transform> xforms;
			const SurfaceShader *shader = nullptr;
			for (const auto& child2: child.children) {
				if (child2.type == "Transform") {
					xforms.emplace_back(parse_matrix(child2.leaf_contents));
				} else if (child2.type == "Data") {
					name = child2.leaf_contents;
				} else if (child2.type == "SurfaceShaderBind") {
					shader = assembly->get_surface_shader(child2.leaf_contents);
					if (shader == nullptr) {
						std::cout << "ERROR: attempted to bind surface shader that doesn't exist." << std::endl;
					}
				}
			}

			// Add instance
			if (assembly->object_map.count(name) != 0) {
				assembly->create_object_instance(name, xforms, shader);
			} else if (assembly->assembly_map.count(name) != 0) {
				assembly->create_assembly_instance(name, xforms, shader);
			} else {
				std::cout << "ERROR: attempted to add instace for data that doesn't exist." << std::endl;
			}
		}
	}

	assembly->optimize();

	return assembly;
}


std::unique_ptr<Bilinear> Parser::parse_bilinear_patch(const DataTree::Node& node) {
	struct BilinearPatchVerts {
		float v[12];
	};

	std::vector<BilinearPatchVerts> patch_verts;

	for (const auto& child: node.children) {
		// Vertex list
		if (child.type == "Vertices") {
			BilinearPatchVerts verts;
			std::sregex_iterator matches(child.leaf_contents.begin(), child.leaf_contents.end(), re_float);
			for (int i = 0; matches != std::sregex_iterator() && i < 12; ++matches) {
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

	return patch;
}


std::unique_ptr<Bicubic> Parser::parse_bicubic_patch(const DataTree::Node& node) {
	struct BicubicPatchVerts {
		float v[48];
	};

	std::vector<BicubicPatchVerts> patch_verts;

	for (const auto& child: node.children) {
		// Vertex list
		if (child.type == "Vertices") {
			BicubicPatchVerts verts;
			std::sregex_iterator matches(child.leaf_contents.begin(), child.leaf_contents.end(), re_float);
			for (int i = 0; matches != std::sregex_iterator() && i < 48; ++matches) {
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

	return patch;
}


std::unique_ptr<SubdivisionSurface> Parser::parse_subdivision_surface(const DataTree::Node& node) {
	// TODO: motion blur for verts
	std::vector<Vec3> verts;
	int vert_count = 0;
	std::vector<int> face_vert_counts;
	std::vector<int> face_vert_indices;

	for (const auto& child: node.children) {
		// Vertex list
		if (child.type == "Vertices") {
			std::sregex_iterator matches(child.leaf_contents.begin(), child.leaf_contents.end(), re_float);
			int i = 0;
			float v_values[3];
			int tot_verts = 0;
			for (; matches != std::sregex_iterator(); ++matches) {
				v_values[i%3] = std::stof(matches->str());
				++i;
				if ((i % 3) == 0) {
					verts.emplace_back(Vec3(v_values[0], v_values[1], v_values[2]));
					++tot_verts;
				}
			}
			if (vert_count == 0) {
				vert_count = tot_verts;
			}
			if (vert_count > 0) {
				verts.resize(verts.size() - (verts.size() % vert_count));
			}
		}
		// Face vertex count list
		else if (child.type == "FaceVertCounts") {
			face_vert_counts.clear();
			std::sregex_iterator matches(child.leaf_contents.begin(), child.leaf_contents.end(), re_int);
			for (; matches != std::sregex_iterator(); ++matches) {
				face_vert_counts.emplace_back(std::stoi(matches->str()));
			}
		}
		// Face vertex index list
		else if (child.type == "FaceVertIndices") {
			face_vert_indices.clear();
			std::sregex_iterator matches(child.leaf_contents.begin(), child.leaf_contents.end(), re_int);
			for (; matches != std::sregex_iterator(); ++matches) {
				face_vert_indices.emplace_back(std::stoi(matches->str()));
			}
		}
	}

	// Build the patch
	std::unique_ptr<SubdivisionSurface> subdiv(new SubdivisionSurface());
	subdiv->set_verts(std::move(verts), vert_count);
	subdiv->set_face_vert_counts(std::move(face_vert_counts));
	subdiv->set_face_vert_indices(std::move(face_vert_indices));

	return subdiv;
}


std::unique_ptr<SurfaceShader> Parser::parse_surface_shader(const DataTree::Node& node) {
	// Find the shader type
	auto shader_type = std::find_if(node.children.cbegin(), node.children.cend(), [](const DataTree::Node& child) {
		return child.type == "Type";
	});
	if (shader_type == node.children.cend()) {
		std::cout << "ERROR: attempted to add surface shader without a type." << std::endl;
		return nullptr;
	}

	if (shader_type->leaf_contents == "Emit") {
		Color col(0.9, 0.9, 0.9);
		for (const auto &child: node.children) {
			if (child.type == "Color") {
				// Get color
				std::sregex_iterator matches(child.leaf_contents.begin(), child.leaf_contents.end(), re_float);
				for (int i = 0; matches != std::sregex_iterator() && i < 3; ++matches) {
					col[i] = std::stof(matches->str());
					++i;
				}
			}
		}

		return std::unique_ptr<SurfaceShader>(new EmitShader {col});
	} else if (shader_type->leaf_contents == "Lambert") {
		Color col(0.9, 0.9, 0.9);
		for (const auto &child: node.children) {
			if (child.type == "Color") {
				// Get color
				std::sregex_iterator matches(child.leaf_contents.begin(), child.leaf_contents.end(), re_float);
				for (int i = 0; matches != std::sregex_iterator() && i < 3; ++matches) {
					col[i] = std::stof(matches->str());
					++i;
				}
			}
		}

		return std::unique_ptr<SurfaceShader>(new LambertShader {col});
	} else if (shader_type->leaf_contents == "GTR") {
		Color col(0.9, 0.9, 0.9);
		float roughness = 0.1;
		float tail_shape = 2.0;
		float fresnel = 0.25;

		for (const auto &child: node.children) {
			if (child.type == "Color") {
				// Get color
				std::sregex_iterator matches(child.leaf_contents.begin(), child.leaf_contents.end(), re_float);
				for (int i = 0; matches != std::sregex_iterator() && i < 3; ++matches) {
					col[i] = std::stof(matches->str());
					++i;
				}
			} else if (child.type == "Roughness") {
				// Get rougness
				std::sregex_iterator matches(child.leaf_contents.begin(), child.leaf_contents.end(), re_float);
				if (matches != std::sregex_iterator()) {
					roughness = std::stof(matches->str());
				}
			} else if (child.type == "TailShape") {
				// Get tail shape
				std::sregex_iterator matches(child.leaf_contents.begin(), child.leaf_contents.end(), re_float);
				if (matches != std::sregex_iterator()) {
					tail_shape = std::stof(matches->str());
				}
			} else if (child.type == "Fresnel") {
				// Get fresnel
				std::sregex_iterator matches(child.leaf_contents.begin(), child.leaf_contents.end(), re_float);
				if (matches != std::sregex_iterator()) {
					fresnel = std::stof(matches->str());
				}
			}
		}

		return std::unique_ptr<SurfaceShader>(new GTRShader {col, roughness, tail_shape, fresnel});
	} else {
		std::cout << "ERROR: unknown surface shader type '" << shader_type->leaf_contents << "'." << std::endl;
		return nullptr;
	}
}


std::unique_ptr<SphereLight> Parser::parse_sphere_light(const DataTree::Node& node) {
	std::vector<Color> colors;
	std::vector<Vec3> locations;
	std::vector<float> radii;

	// Parse
	for (const auto& child: node.children) {
		if (child.type == "Color") {
			// Get color
			std::sregex_iterator matches(child.leaf_contents.begin(), child.leaf_contents.end(), re_float);
			Color col;
			for (int i = 0; matches != std::sregex_iterator() && i < 3; ++matches) {
				col[i] = std::stof(matches->str());
				++i;
			}
			colors.push_back(col);
		} else if (child.type == "Radius") {
			// Get radius
			std::sregex_iterator matches(child.leaf_contents.begin(), child.leaf_contents.end(), re_float);
			if (matches != std::sregex_iterator()) {
				radii.emplace_back(std::stof(matches->str()));
			}
		} else if (child.type == "Location") {
			// Get location
			std::sregex_iterator matches(child.leaf_contents.begin(), child.leaf_contents.end(), re_float);
			Vec3 loc;
			for (int i = 0; matches != std::sregex_iterator() && i < 3; ++matches) {
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


std::unique_ptr<RectangleLight> Parser::parse_rectangle_light(const DataTree::Node& node) {
	std::vector<std::pair<float, float>> dimensions;
	std::vector<Color> colors;

	// Parse
	for (const auto& child: node.children) {
		if (child.type == "Color") {
			// Get color
			std::sregex_iterator matches(child.leaf_contents.begin(), child.leaf_contents.end(), re_float);
			Color col;
			for (int i = 0; matches != std::sregex_iterator() && i < 3; ++matches) {
				col[i] = std::stof(matches->str());
				++i;
			}
			colors.push_back(col);
		} else if (child.type == "Dimensions") {
			// Get dimensions
			float dim_x = 1.0f;
			float dim_y = 1.0f;
			std::sregex_iterator matches(child.leaf_contents.begin(), child.leaf_contents.end(), re_float);
			if (matches != std::sregex_iterator()) {
				dim_x = std::stof(matches->str());
			}
			++matches;
			if (matches != std::sregex_iterator()) {
				dim_y = std::stof(matches->str());
			}
			dimensions.emplace_back(std::make_pair(dim_x, dim_y));
		}
	}

	// Build light
	std::unique_ptr<RectangleLight> rl(new RectangleLight(dimensions, colors));

	return rl;
}


std::unique_ptr<Sphere> Parser::parse_sphere(const DataTree::Node& node) {
	// TODO: motion blur for spheres
	Vec3 location {0,0,0};
	float radius {0.5f};

	// Parse
	for (const auto& child: node.children) {
		if (child.type == "Radius") {
			// Get radius
			std::sregex_iterator matches(child.leaf_contents.begin(), child.leaf_contents.end(), re_float);
			if (matches != std::sregex_iterator()) {
				radius = std::stof(matches->str());
			}
		} else if (child.type == "Location") {
			// Get location
			std::sregex_iterator matches(child.leaf_contents.begin(), child.leaf_contents.end(), re_float);
			for (int i = 0; matches != std::sregex_iterator() && i < 3; ++matches) {
				location[i] = std::stof(matches->str());
				++i;
			}
		}
	}

	// Build sphere
	std::unique_ptr<Sphere> s(new Sphere(location, radius));

	return s;
}
