#ifndef PARSER_HPP
#define PARSER_HPP

#include <string>
#include <fstream>

#include "sphere_light.hpp"
#include "bilinear.hpp"

#include "renderer.hpp"
#include "scene.hpp"


class Parser
{
	std::ifstream psy_file;

	// Methods

	/**
	 * @brief Parses the frame header.
	 */
	std::tuple<int, int, int, int, std::string> parse_frame_header();

	/**
	 * @brief Parses a camera section.
	 */
	Camera *parse_camera();

	/**
	 * @brief Parses a bilinear patch section.
	 */
	Bilinear *parse_bilinear_patch();

	/**
	 * @brief Parses a sphere light section.
	 */
	SphereLight *parse_sphere_light();

public:
	Parser(std::string filename) {
		psy_file.open(filename);
	}

	/**
	 * @brief Parses the next frame in the file, and returns the
	 * resulting scene, ready for rendering.
	 */
	Renderer *parse_next_frame();
};

#endif // PARSER_HPP