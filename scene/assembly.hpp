#ifndef ASSEMBLY_HPP
#define ASSEMBLY_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

#include "numtype.h"
#include "global.hpp"
#include "utils.hpp"
#include "bbox.hpp"
#include "transform.hpp"
#include "bvh.hpp"
#include "bvh2.hpp"
#include "bvh4.hpp"
#include "light_array.hpp"
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

	std::string to_string() const {
		std::string s;
		s.append("Type: ");
		switch (type) {
			case OBJECT:
				s.append("OBJECT");
				break;
			case ASSEMBLY:
				s.append("ASSEMBLY");
				break;
			default:
				s.append("Unknown");
				break;
		}
		s.append("\nData Index: ");
		s.append(std::to_string(data_index));
		s.append("\nTransform Index: ");
		s.append(std::to_string(transform_index));
		s.append("\nTransform Count: ");
		s.append(std::to_string(transform_count));
		s.append("\n");

		return s;
	}
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
	BVH4 object_accel;

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
	 * Optimizes the contents of an assembly for maximum ray tracing
	 * performance and memory usage.
	 *
	 * This is not required to be run at all, but if it is run it needs
	 * to be run _before_ finalize().
	 */
	void optimize() {
	}

	/**
	 * Prepares the assembly to be used for rendering.
	 */
	bool finalize() {
		// Finalize all sub-assemblies
		for (auto& ass: assemblies) {
			ass->finalize();
		}

		// Clear maps (no longer needed)
		object_map.clear();
		assembly_map.clear();
		//shader_map.clear();

		// Shrink storage to minimum
		instances.shrink_to_fit();
		xforms.shrink_to_fit();
		objects.shrink_to_fit();
		object_map.rehash(0);
		assemblies.shrink_to_fit();
		assembly_map.rehash(0);

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


	/**
	 * Calculates and returns the proper transformed bounding boxes of an
	 * instance.
	 */
	std::vector<BBox> instance_bounds(size_t index) const {
		std::vector<BBox> bbs;

		// Get bounding boxes
		if (instances[index].type == Instance::OBJECT) {
			auto obj = objects[instances[index].data_index].get();
			for (unsigned int i = 0; i < obj->bounds().size(); ++i) {
				bbs.push_back(obj->bounds()[i]);
			}
		} else { /* Instance::ASSEMBLY */
			auto asmb = assemblies[instances[index].data_index].get();
			bbs = asmb->object_accel.bounds();
		}

		// Transform the bounding boxes
		auto xstart = xforms.cbegin() + instances[index].transform_index;
		auto xend = xstart + instances[index].transform_count;
		bbs = transform_from(bbs, xstart, xend);

		return bbs;
	}


	/**
	 * Calculates and returns the bounds of an instance at a particular moment
	 * in time.
	 */
	BBox instance_bounds_at(float t, size_t index) const {
		BBox bb;

		// Calculate bounds at time t
		if (instances[index].type == Instance::OBJECT) {
			// Get BBox at time t
			bb = lerp_seq(t, objects[instances[index].data_index]->bounds());
		} else { /* Instance::ASSEMBLY */
			// Get BBox at time t
			const auto& bbs = assemblies[instances[index].data_index]->object_accel.bounds();
			auto begin = bbs.begin();
			auto end = bbs.end();
			bb = lerp_seq(t, begin, end);
		}

		// Transform bounds if necessary
		if (instances[index].transform_count > 0) {
			// Get bounds and center at time t
			auto xstart = xforms.begin() + instances[index].transform_index;
			auto xend = xstart + instances[index].transform_count;
			bb = bb.inverse_transformed(lerp_seq(t, xstart, xend));
		}

		return bb;
	}


	/**
	 * Calculates and returns the transforms of an instance at a particular moment
	 * in time.
	 */
	Transform instance_xform_at(float t, size_t index) const {
		// Transform bounds if necessary
		if (instances[index].transform_count > 0) {
			// Get bounds and center at time t
			const auto tb = xforms.begin() + instances[index].transform_index;
			const auto te = tb + instances[index].transform_count;
			return lerp_seq(t, tb, te);
		} else {
			return Transform();
		}
	}
};

#endif // ASSEMBLY_HPP
