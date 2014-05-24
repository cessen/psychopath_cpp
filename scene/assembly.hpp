#ifndef ASSEMBLY_HPP
#define ASSEMBLY_HPP

#include <vector>
#include <unordered_map>
#include <memory>

#include "numtype.h"
#include "global.hpp"
#include "utils.hpp"
#include "bbox.hpp"
#include "transform.hpp"
#include "bvh.hpp"
#include "light_tree.hpp"


/**
 * Represents an instance of an object or assembly within an
 * assembly.
 */
struct Instance {
	enum Type {
	    OBJECT,
	    ASSEMBLY
	};

	Type type; // The type of the thing being instanced

	size_t data_index; // Index of the thing being instanced in the array of its type

	size_t transform_index; // Index of the first transform for this instance in the transforms array
	size_t transform_count; // The number of transforms, for transformation motion blur. If zero, no transforms.
};


/**
 * An Assembly is a self-contained set of geometry, lights, and
 * shaders.  Objects in assemblies do not have any transform
 * hierarchy: individual objects have completely independent
 * transforms.
 */
class Assembly
{
public:
	// Instance list
	std::vector<Instance> instances;
	std::vector<Transform> xforms;

	// Object list
	std::vector<std::unique_ptr<Object>> objects;
	std::unordered_map<std::string, size_t> object_map; // map Name -> Index

	// Assembly list
	std::vector<std::unique_ptr<Assembly>> assemblies;
	std::unordered_map<std::string, size_t> assembly_map; // map Name -> Index

	// TODO: Shader list
	//std::vector<SHADER> shaders;
	//std::unordered_map<std::string, size_t> shader_map; // map Name -> Index

	// Object accel
	BVH object_accel;

	// Light accel
	LightTree light_accel;

	/*****************************************************/

	/**
	 * Adds an object to the assembly.
	 *
	 * Note that this does not add the object in such a way that it will be
	 * rendered.  To make the object render, you also must instance it in the
	 * assembly with create_object_instance().
	 */
	bool add_object(const std::string& name, std::unique_ptr<Object>&& object) {
		object->uid = ++Global::next_object_uid; // TODO: use implicit ID's derived from scene hierarchy.
		objects.emplace_back(std::move(object));
		object_map.emplace(name, objects.size() -1);

		return true;
	}


	/**
	* Adds a sub-assembly to the assembly.
	*
	* Note that this does not add the sub-assembly in such a way that it will
	* be rendered.  To make the sub-assembly render, you also must instance it
	* in the assembly with create_assembly_instance().
	*/
	bool add_assembly(const std::string& name, std::unique_ptr<Assembly>&& assembly) {
		//assembly->uid = ++Global::next_object_uid; // TODO: use implicit ID's derived from scene hierarchy.
		assemblies.emplace_back(std::move(assembly));
		assembly_map.emplace(name, assemblies.size() -1);

		return true;
	}


	/**
	 * Creates an instance of an already added object.
	 */
	bool create_object_instance(const std::string& name, const std::vector<Transform>& transforms) {
		// Add the instance
		instances.emplace_back(Instance {Instance::OBJECT, object_map[name], xforms.size(), transforms.size()});

		// Add transforms
		for (const auto& trans: transforms) {
			xforms.emplace_back(trans);
		}

		return true;
	}


	/**
	* Creates an instance of an already added assembly.
	*/
	bool create_assembly_instance(const std::string& name, const std::vector<Transform>& transforms) {
		// Add the instance
		instances.emplace_back(Instance {Instance::ASSEMBLY, assembly_map[name], xforms.size(), transforms.size()});

		// Add transforms
		for (const auto& trans: transforms) {
			xforms.emplace_back(trans);
		}

		return true;
	}


	/**
	 * Prepares the assembly to be used for rendering.
	 */
	bool finalize() {
		// Clear maps (no longer needed)
		object_map.clear();
		assembly_map.clear();
		//shader_map.clear();

		// Build object accel
		object_accel.build(*this);

		// Build light accel
		light_accel.build(*this);

		return true;
	}


	/**
	 * Returns the number of bits needed to give each scene
	 * element in the assembly a unique integer id.
	 */
	size_t element_id_bits() const {
		// TODO: both intlog2 and upper_power_of_two operate on 32-bit unsigned ints.
		// That's not enough for size_t.  For now it's fine, as I doubt I'll actually
		// be rendering scenes with that many objects any time soon.  But it will need
		// to be changed eventually.

		// TODO: also, the element_id_bits should be cached in the assembly, so it
		// doesn't need to be calculated on the fly over and over and over again.
		return intlog2(upper_power_of_two(instances.size()));
	}

	//std::vector<BBox> bounds() const;
};

#endif // ASSEMBLY_HPP
