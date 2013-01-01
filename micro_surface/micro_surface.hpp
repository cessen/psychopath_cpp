#ifndef MICRO_SURFACE_HPP
#define MICRO_SURFACE_HPP

#include "numtype.h"

#include <vector>

#include "vector.hpp"
#include "bbox.hpp"
#include "grid.hpp"
#include "ray.hpp"
#include "intersection.hpp"
#include "utils.hpp"
#include "rng.hpp"


/**
 * @brief A node in the MicroSurface tree.
 *
 * The MicroSurface tree is essentially a BVH, except where the BBoxes
 * of the leaf nodes _are_ the geometry to be tested against.
 */
struct MicroNode {
	BBox bounds;
	uint16 flags;

	uint16 child_index;

	uint16 data_index; // Index into geometry data
	uint8 data_du; // Deltas for the extent of geometry data that this node covers
	uint8 data_dv;
};


/**
 * @brief Lowest-common-denominator representation of a surface.
 *
 * All surfaces are eventually converted to a MicroSurface before direct
 * ray testing.
 */
class MicroSurface
{
	// MicroSurface tree
	std::vector<MicroNode> nodes;
	uint_i res_u, res_v;

	// Important geometry information
	std::vector<Vec3> normals;
	std::vector<float32> uvs;
	uint_i face_id;

	// Number of time samples
	uint16 time_count;

	// Random number generator
	RNG rng;

	/**
	 * @brief Calculates ray-bbox intersection with a node in the
	 * MicroSurface tree.
	 */
	bool intersect_node(uint_i node, const Ray &ray, float32 *tnear, float32 *tfar, float32 *t) {
		uint32 ti = 0;
		float32 alpha = 0.0f;
		if (calc_time_interp(time_count, ray.time, &ti, &alpha)) {
			const BBox b = lerp<BBox>(alpha, nodes[node+ti].bounds, nodes[node+ti+1].bounds);
			return b.intersect_ray(ray, tnear, tfar, t);
		} else {
			return nodes[node+ti].bounds.intersect_ray(ray, tnear, tfar, t);
		}
	}

public:
	// Constructors
	MicroSurface() {}
	MicroSurface(Grid *grid) {
		init_from_grid(grid);
	}


	/**
	 * @brief Initializes the MicroSurface from a grid.
	 *
	 * The grid will be modified during this process, but discarded
	 * when finished.  Surface normals will be generated, displacements
	 * calculated, etc.
	 */
	void init_from_grid(Grid *grid);


	/**
	 * @brief Intersects a ray with the MicroSurface.
	 *
	 * @param ray The ray to test against.
	 * @param inter If not null, resultant intersection data is
	 *              stored in here.  If null, this function behaves
	 *              as a simple occulsion test.
	 *
	 * @return True on a hit, false on a miss.
	 */
	bool intersect_ray(const Ray &ray, Intersection *inter);


	/**
	 * @brief Returns how much memory this MicroSurface occupies.
	 */
	uint_i bytes() const {
		const uint_i class_size = sizeof(MicroSurface);
		const uint_i nodes_size = sizeof(MicroNode) * nodes.size();
		const uint_i normals_size = sizeof(Vec3) * normals.size();
		const uint_i uvs_size = sizeof(float32) * uvs.size();

		return class_size + nodes_size + normals_size + uvs_size;
	}
};

#endif // MICRO_SURFACE_HPP