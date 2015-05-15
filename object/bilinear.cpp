#include "numtype.h"

#include <algorithm>
#include <iostream>
#include <limits>
#include <stdlib.h>
#include <cmath>
#include "bilinear.hpp"
#include "config.hpp"
#include "global.hpp"




Bilinear::Bilinear(Vec3 v1, Vec3 v2, Vec3 v3, Vec3 v4)
{
	verts.push_back( {{v1,v2,v3,v4}});
}

void Bilinear::finalize()
{
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


void Bilinear::add_time_sample(Vec3 v1, Vec3 v2, Vec3 v3, Vec3 v4)
{
	verts.push_back( {{v1,v2,v3,v4}});
}


const std::vector<BBox> &Bilinear::bounds() const
{
	return bbox;
}
