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

	uint32_t child_index;

	uint16_t data_index; // Index into geometry data
	uint8_t data_du; // Deltas for the extent of geometry data that this node covers
	uint8_t data_dv;

	uint8_t flags;
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
	size_t res_u, res_v;

	// Important geometry information
	std::vector<Vec3> verts;
	std::vector<Vec3> normals;
	std::vector<float> uvs;
	size_t face_id;

	// Number of time samples
	uint16_t time_count;

	// Max width of the surface at the root node
	float root_width;

	/**
	 * @brief Calculates ray-bbox intersection with a node in the
	 * MicroSurface tree.
	 */
	bool intersect_node(size_t node, const Ray &ray, float *tnear, float *tfar, float t) {
		uint32_t ti = 0;
		float alpha = 0.0f;
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
	 * @brief Returns the number of subdivisions used to create
	 * this MicroSurface.
	 */
	size_t subdivisions() const {
		return intlog2(res_u);
	}

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
	bool intersect_ray(const Ray &ray, float width, Intersection *inter, RNG* rng=nullptr);


	/**
	 * @brief Returns how much memory this MicroSurface occupies.
	 */
	size_t bytes() const {
		const size_t class_size = sizeof(MicroSurface);
		const size_t nodes_size = sizeof(MicroNode) * nodes.size();
		const size_t normals_size = sizeof(Vec3) * normals.size();
		const size_t uvs_size = sizeof(float) * uvs.size();

		return class_size + nodes_size + normals_size + uvs_size;
	}
};


static inline size_t size_in_bytes(const MicroSurface& data)
{
	return data.bytes();
}

#endif // MICRO_SURFACE_HPP
