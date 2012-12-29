#ifndef GRID_HPP
#define GRID_HPP

#include <vector>

#include "numtype.h"
#include "vector.hpp"


/*
 * A micropolygon grid.
 */
struct Grid {
	// Resolution
	uint16 res_u, res_v; // In vertices, not faces
	uint16 time_count;

	// Data
	std::vector<Vec3> verts; // v1_t1, v1_t2, v1_t3, v2_t1, v2_t2, v2_t3...
	float32 u1, v1,
	        u2, v2,
	        u3, v3,
	        u4, v4;
	uint_i face_id;

	// Constructors
	Grid() {}
	Grid(uint16 ru, uint16 rv, uint16 tc) {
		res_u = ru;
		res_v = rv;
		time_count = tc;

		verts.resize(ru*rv*tc);
	}

	// Convenience methods
	/**
	 * @brief Computes surface normals for each vertex of the grid.
	 *
	 * The normals are stored in the following order:
	 * n1_t1, n1_t2, n1_t3, n2_t1, n2_t2, n2_t3...
	 *
	 * @param normals Pointer to already allocated space to store
	 *                the normals.
	 */
	bool calc_normals(Vec3 *normals);

	/**
	 * @brief Computes uv coordinates for each vertex of the grid.
	 *
	 * The coordinates are stored in the following order:
	 * u1, v1, u2, v2, u3, v3...
	 *
	 * @param uvs Pointer to already allocated space to store the uvs.
	 */
	bool calc_uvs(float32 *uvs);
};

#endif
