/*
 * This file and scene.cpp define a Scene class, which is used to build and
 * store a scene description to be rendered.
 */
#ifndef SCENE_HPP
#define SCENE_HPP

#include "numtype.h"

#include <vector>

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
	Camera *camera;
	std::vector<Primitive *> primitives;
	std::vector<Light *> finite_lights;
	BVH world;

	Scene(): camera {nullptr} {}

	~Scene() {
		uint32 s;

		// Delete finite lights
		s = finite_lights.size();
		for (uint32 i = 0; i < s; i++) {
			delete finite_lights[i];
		}

		// Delete camera
		if (camera)
			delete camera;
	}


	void add_primitive(Primitive *primitive) {
		primitives.push_back(primitive);
	}

	void add_finite_light(Light *light) {
		finite_lights.push_back(light);
	}

	// Finalizes the scene for rendering
	void finalize() {
		world.add_primitives(primitives);
		world.finalize();
	}

	bool intersect_ray(Ray &ray, Intersection *intersection=nullptr) {
		return world.intersect_ray(ray, intersection);
	}
};

#endif // SCENE_H
