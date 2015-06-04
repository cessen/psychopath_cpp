#ifndef PARSER_HPP
#define PARSER_HPP

#include <string>
#include <fstream>
#include <memory>

#include "data_tree.hpp"

#include "sphere_light.hpp"
#include "rectangle_light.hpp"
#include "bilinear.hpp"
#include "bicubic.hpp"
#include "sphere.hpp"

#include "renderer.hpp"
#include "scene.hpp"


class Parser
{
	DataTree::Node tree;
	unsigned int node_index = 0;

	// Methods

	/**
	* @brief Parses a transform matrix.
	*/
	Matrix44 parse_matrix(const std::string line);

	/**
	* @brief Parses a Camera section.
	*/
	std::unique_ptr<Camera> parse_camera(const DataTree::Node& node);

	/**
	 * @brief Parses an Assembly section.
	 */
	std::unique_ptr<Assembly> parse_assembly(const DataTree::Node& node, const Assembly* parent_assembly);


	/**
	 * @brief Parses a bilinear patch section.
	 */
	std::unique_ptr<Bilinear> parse_bilinear_patch(const DataTree::Node& node);

	/**
	 * @brief Parses a bicubic patch section.
	 */
	std::unique_ptr<Bicubic> parse_bicubic_patch(const DataTree::Node& node);

	/**
	 * @brief Parses a sphere section.
	 */
	std::unique_ptr<Sphere> parse_sphere(const DataTree::Node& node);

	/**
	 * @brief Parses a surface shader section.
	 */
	std::unique_ptr<SurfaceShader> parse_surface_shader(const DataTree::Node& node);

	/**
	 * @brief Parses a sphere light section.
	 */
	std::unique_ptr<SphereLight> parse_sphere_light(const DataTree::Node& node);

	/**
	 * @brief Parses a rectangle light section.
	 */
	std::unique_ptr<RectangleLight> parse_rectangle_light(const DataTree::Node& node);

public:
	Parser(std::string input_path) {
		tree = DataTree::build_from_file(input_path.c_str());
		//DataTree::print_tree(tree);
	}

	/**
	 * @brief Parses the next frame in the file, and returns the
	 * resulting scene, ready for rendering.
	 */
	std::unique_ptr<Renderer> parse_next_frame();
};

#endif // PARSER_HPP
