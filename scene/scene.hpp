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
#include "scene_graph.hpp"

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
	// Scene description
	std::unique_ptr<Camera> camera;
	SceneGraph scene_graph;

	// Scene acceleration structures, generated after the scene is fully
	// described
	BVH object_accel;
	std::vector<Light*> finite_light_accel;

	Scene() {}

	/**
	 * @brief Adds a collection to the scene.
	 *
	 * There must be at least one "ROOT" collection in the scene.
	 */
	void add_collection(std::string name, std::unique_ptr<Collection>&& collection) {
		scene_graph.collections.emplace(name, std::move(collection));
	}

	/**
	 * @brief Adds an object to the scene.
	 *
	 * Note that for the object to actually be instantiated in the scene,
	 * it needs to be referenced in a Collection.
	 */
	void add_object(std::string name, std::unique_ptr<Object>&& object) {
		object->uid = ++Global::next_object_uid;
		scene_graph.objects.emplace(name, std::move(object));
	}

	/**
	 * @brief Adds a finite light to the scene.
	 */
	void add_finite_light(std::string name, std::unique_ptr<Light>&& light) {
		scene_graph.finite_lights.emplace(name, std::move(light));
	}

	// Finalizes the scene for rendering
	void finalize() {
		std::cout << "Finalizing scene with " << scene_graph.objects.size() << " objects." << std::endl;
		object_accel.build(scene_graph);

		//std::cout << object_accel. << std::endl;

		// TODO: use a real light acceleration structure
		for (auto& l: scene_graph.finite_lights) {
			finite_light_accel.push_back(l.second.get());
		}
	}
};

#endif // SCENE_H
