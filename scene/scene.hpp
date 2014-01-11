/*
 * This file and scene.cpp define a Scene class, which is used to build and
 * store a scene description to be rendered.
 */
#ifndef SCENE_HPP
#define SCENE_HPP

#include "numtype.h"

#include <vector>
#include <memory>

#include "camera.hpp"
#include "bvh.hpp"
#include "prim_array.hpp"
#include "primitive.hpp"
#include "light.hpp"

/**
 * @brief A 3D scene for rendering.
 *
 * The Scene class is used to build and store the complete description of a 3d
 * scene to be rendered.
 *
 * The scene is built via the various methods to add primitives, lights, and
 * shaders.  It must be finalized (which initializes important acceleration
 * structures, etc.) before being passed off for rendering.
 */
struct Scene {
	std::unique_ptr<Camera> camera;
	std::vector<std::unique_ptr<Primitive>> primitives;
	std::vector<std::unique_ptr<Light>> finite_lights;
	//PrimArray world;
	BVH world;

	Scene() {}




	void add_primitive(std::unique_ptr<Primitive>&& primitive) {
		primitives.push_back(std::move(primitive));
	}

	void add_finite_light(std::unique_ptr<Light>&& light) {
		finite_lights.push_back(std::move(light));
	}

	// Finalizes the scene for rendering
	void finalize() {
		world.add_primitives(&primitives);
		world.finalize();
	}
};

#endif // SCENE_H
