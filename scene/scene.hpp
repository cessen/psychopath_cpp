/*
 * This file and scene.cpp define a Scene class, which is used to build and
 * store a scene description to be rendered.
 */
#ifndef SCENE_HPP
#define SCENE_HPP

#include "numtype.h"

#include "global.hpp"
#include "camera.hpp"
#include "bvh.hpp"
#include "light_array.hpp"
#include "light_tree.hpp"
#include "scene_graph.hpp"
#include "assembly.hpp"

#include <vector>
#include <unordered_map>
#include <string>
#include <memory>

#include <iostream>




/**
 * @brief A 3D scene for rendering.
 *
 * The Scene class is used to build and store the complete description of a 3d
 * scene to be rendered.
 */
struct Scene {
	std::unique_ptr<Camera> camera; // The camera of the scene

	std::unique_ptr<Assembly> root; // The root assembly of the scene


	Scene() {}

	/**
	 * @brief Sets the root assembly of the scene.
	 */
	void set_root_assembly(std::unique_ptr<Assembly>&& root_) {
		root = std::move(root_);
	}

	// Finalizes the scene for rendering
	void finalize() {
	}
};

#endif // SCENE_H
