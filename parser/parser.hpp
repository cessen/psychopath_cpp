#ifndef PARSER_HPP
#define PARSER_HPP

#include <string>
#include <fstream>
#include <memory>

#include "sphere_light.hpp"
#include "bilinear.hpp"
#include "bicubic.hpp"

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
	std::unique_ptr<Camera> parse_camera();

	/**
	 * @brief Parses a bilinear patch section.
	 */
	std::unique_ptr<Bilinear> parse_bilinear_patch();

	/**
	 * @brief Parses a bicubic patch section.
	 */
	std::unique_ptr<Bicubic> parse_bicubic_patch();

	/**
	 * @brief Parses a sphere light section.
	 */
	std::unique_ptr<SphereLight> parse_sphere_light();

public:
	Parser(std::string filename) {
		psy_file.open(filename);
	}

	/**
	 * @brief Parses the next frame in the file, and returns the
	 * resulting scene, ready for rendering.
	 */
	std::unique_ptr<Renderer> parse_next_frame();
};

#endif // PARSER_HPP