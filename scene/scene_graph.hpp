#ifndef SCENE_GRAPH_HPP
#define SCENE_GRAPH_HPP

#include "transform.hpp"
#include "object.hpp"
#include "light.hpp"

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

struct Collection {
	std::vector<Transform> xform; // The transforms of the collection

	std::vector<std::string> objects; // The names of the objects in the collection
	std::vector<std::string> collections; // The names of the collections in the collection

	Collection() {}
	Collection(std::vector<std::string> objs, std::vector<std::string> cols): objects {objs}, collections {cols} {}
};

struct SceneGraph {
	std::unordered_map<std::string, std::unique_ptr<Collection>> collections;
	std::unordered_map<std::string, std::unique_ptr<Object>> objects;
	std::unordered_map<std::string, std::unique_ptr<Light>> finite_lights;
	//std::unordered_map<std::string, std::unique_ptr<Shader>> shaders;
};

#endif // SCENE_GRAPH_HPP