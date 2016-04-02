#include "numtype.h"

#include <algorithm>
#include <iostream>
#include <limits>
#include <stdlib.h>
#include <cmath>
#include <utility>
#include <vector>
#include "stack.hpp"
#include "bicubic.hpp"
#include "config.hpp"
#include "global.hpp"

#include "surface_closure.hpp"
#include "closure_union.hpp"




Bicubic::Bicubic(Vec3 v1,  Vec3 v2,  Vec3 v3,  Vec3 v4,
                 Vec3 v5,  Vec3 v6,  Vec3 v7,  Vec3 v8,
                 Vec3 v9,  Vec3 v10, Vec3 v11, Vec3 v12,
                 Vec3 v13, Vec3 v14, Vec3 v15, Vec3 v16) {
	verts.resize(1);

	verts[0][0]  = v1;
	verts[0][1]  = v2;
	verts[0][2]  = v3;
	verts[0][3]  = v4;

	verts[0][4]  = v5;
	verts[0][5]  = v6;
	verts[0][6]  = v7;
	verts[0][7]  = v8;

	verts[0][8]  = v9;
	verts[0][9]  = v10;
	verts[0][10] = v11;
	verts[0][11] = v12;

	verts[0][12] = v13;
	verts[0][13] = v14;
	verts[0][14] = v15;
	verts[0][15] = v16;
}


void Bicubic::add_time_sample(Vec3 v1,  Vec3 v2,  Vec3 v3,  Vec3 v4,
                              Vec3 v5,  Vec3 v6,  Vec3 v7,  Vec3 v8,
                              Vec3 v9,  Vec3 v10, Vec3 v11, Vec3 v12,
                              Vec3 v13, Vec3 v14, Vec3 v15, Vec3 v16) {
	const auto i = verts.size();
	verts.resize(verts.size()+1);

	verts[i][0]  = v1;
	verts[i][1]  = v2;
	verts[i][2]  = v3;
	verts[i][3]  = v4;

	verts[i][4]  = v5;
	verts[i][5]  = v6;
	verts[i][6]  = v7;
	verts[i][7]  = v8;

	verts[i][8]  = v9;
	verts[i][9]  = v10;
	verts[i][10] = v11;
	verts[i][11] = v12;

	verts[i][12] = v13;
	verts[i][13] = v14;
	verts[i][14] = v15;
	verts[i][15] = v16;
}

void Bicubic::add_time_sample(std::array<Vec3, 16> patch) {
	verts.emplace_back(patch);
}


void Bicubic::finalize() {
	// Calculate bounds
	bbox.resize(verts.size());
	for (size_t time = 0; time < verts.size(); time++) {
		bbox[time] = bound(verts[time]);

		// Extend bounds for displacements
		for (int i = 0; i < 3; i++) {
			bbox[time].min[i] -= Config::displace_distance;
			bbox[time].max[i] += Config::displace_distance;
		}
	}
}


const std::vector<BBox> &Bicubic::bounds() const {
	return bbox;
}

