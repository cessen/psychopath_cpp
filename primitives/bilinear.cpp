#include "numtype.h"

#include <algorithm>
#include <iostream>
#include <limits>
#include <stdlib.h>
#include "bilinear.hpp"
#include "grid.hpp"
#include "config.hpp"
#include "global.hpp"


Bilinear::Bilinear(Vec3 v1, Vec3 v2, Vec3 v3, Vec3 v4)
{
	verts.push_back( {{v1,v2,v3,v4}});

	u_min = v_min = 0.0f;
	u_max = v_max = 1.0f;
}

void Bilinear::finalize()
{
	// Calculate longest sides of the patch
	longest_u = (verts[0][0] - verts[0][1]).length() > (verts[0][2] - verts[0][3]).length() ? (verts[0][0] - verts[0][1]).length() : (verts[0][2] - verts[0][3]).length();
	longest_v = (verts[0][0] - verts[0][3]).length() > (verts[0][1] - verts[0][2]).length() ? (verts[0][0] - verts[0][3]).length() : (verts[0][1] - verts[0][2]).length();

	// Calculate bounds
	bbox.init(verts.size());
	for (int time = 0; time < verts.size(); time++) {
		bbox[time].min.x = verts[time][0].x;
		bbox[time].max.x = verts[time][0].x;
		bbox[time].min.y = verts[time][0].y;
		bbox[time].max.y = verts[time][0].y;
		bbox[time].min.z = verts[time][0].z;
		bbox[time].max.z = verts[time][0].z;

		for (int i = 1; i < 4; i++) {
			bbox[time].min.x = verts[time][i].x < bbox[time].min.x ? verts[time][i].x : bbox[time].min.x;
			bbox[time].max.x = verts[time][i].x > bbox[time].max.x ? verts[time][i].x : bbox[time].max.x;
			bbox[time].min.y = verts[time][i].y < bbox[time].min.y ? verts[time][i].y : bbox[time].min.y;
			bbox[time].max.y = verts[time][i].y > bbox[time].max.y ? verts[time][i].y : bbox[time].max.y;
			bbox[time].min.z = verts[time][i].z < bbox[time].min.z ? verts[time][i].z : bbox[time].min.z;
			bbox[time].max.z = verts[time][i].z > bbox[time].max.z ? verts[time][i].z : bbox[time].max.z;
		}

		// Extend bounds for displacements
		for (int i = 1; i < 4; i++) {
			bbox[time].min.x -= Config::displace_distance;
			bbox[time].max.x += Config::displace_distance;
			bbox[time].min.y -= Config::displace_distance;
			bbox[time].max.y += Config::displace_distance;
			bbox[time].min.z -= Config::displace_distance;
			bbox[time].max.z += Config::displace_distance;
		}
	}
}


void Bilinear::add_time_sample(Vec3 v1, Vec3 v2, Vec3 v3, Vec3 v4)
{
	verts.push_back( {{v1,v2,v3,v4}});
}


//////////////////////////////////////////////////////////////

size_t Bilinear::subdiv_estimate(float width) const
{
	if (width < Config::min_upoly_size)
		width = Config::min_upoly_size;

	// Power-of-two dicing rate
	size_t rate = (std::max(longest_u, longest_v) / (width * Config::dice_rate)) + 1;
	rate = intlog2(upper_power_of_two(rate));

	return rate;
}


BBoxT &Bilinear::bounds()
{
	return bbox;
}


int Bilinear::split(std::unique_ptr<DiceableSurfacePrimitive> primitives[])
{
	auto patch1 = new Bilinear();
	auto patch2 = new Bilinear();

	// Split
	if (longest_u > longest_v) {
		// Split on U
		for (int i=0; i < verts.size(); i++) {
			patch1->add_time_sample(verts[i][0],
			                        (verts[i][0] + verts[i][1])*0.5,
			                        (verts[i][2] + verts[i][3])*0.5,
			                        verts[i][3]
			                       );
			patch2->add_time_sample((verts[i][0] + verts[i][1])*0.5,
			                        verts[i][1],
			                        verts[i][2],
			                        (verts[i][2] + verts[i][3])*0.5
			                       );
		}

		// Fill in uv's
		patch1->u_min = u_min;
		patch1->u_max = (u_min + u_max) / 2;
		patch1->v_min = v_min;
		patch1->v_max = v_max;

		patch2->u_min = (u_min + u_max) / 2;
		patch2->u_max = u_max;
		patch2->v_min = v_min;
		patch2->v_max = v_max;
	} else {
		// Split on V
		for (int i=0; i < verts.size(); i++) {
			patch1->add_time_sample(verts[i][0],
			                        verts[i][1],
			                        (verts[i][1] + verts[i][2])*0.5,
			                        (verts[i][3] + verts[i][0])*0.5
			                       );
			patch2->add_time_sample((verts[i][3] + verts[i][0])*0.5,
			                        (verts[i][1] + verts[i][2])*0.5,
			                        verts[i][2],
			                        verts[i][3]
			                       );
		}

		// Fill in uv's
		patch1->u_min = u_min;
		patch1->u_max = u_max;
		patch1->v_min = v_min;
		patch1->v_max = (v_min + v_max) / 2;

		patch2->u_min = u_min;
		patch2->u_max = u_max;
		patch2->v_min = (v_min + v_max) / 2;
		patch2->v_max = v_max;
	}

	patch1->finalize();
	patch2->finalize();

	primitives[0] = std::unique_ptr<DiceableSurfacePrimitive>(patch1);
	primitives[1] = std::unique_ptr<DiceableSurfacePrimitive>(patch2);

	return 2;
}

std::unique_ptr<DiceableSurfacePrimitive> Bilinear::copy()
{
	auto patch = new Bilinear();

	// Copy verts
	patch->verts = verts;

	// Copy uv's
	patch->u_min = u_min;
	patch->u_max = u_max;
	patch->v_min = v_min;
	patch->v_max = v_max;

	// Copy pre-calculated information
	patch->bbox = bbox;
	patch->longest_u = longest_u;
	patch->longest_v = longest_v;

	return std::unique_ptr<DiceableSurfacePrimitive>(patch);
}


std::shared_ptr<MicroSurface> Bilinear::dice(size_t subdivisions)
{
	// Get dicing rate
	size_t u_rate = 1 << subdivisions;
	size_t v_rate = 1 << subdivisions;

	// Dice away!
	std::unique_ptr<Grid> grid = std::unique_ptr<Grid>(grid_dice(u_rate+1, v_rate+1));
	std::shared_ptr<MicroSurface> micro = std::make_shared<MicroSurface>(grid.get());

	return micro;
}


/*
 * Dice the patch into a micropoly grid.
 * ru and rv are the resolution of the grid in vertices
 * in the u and v directions.
 */
Grid *Bilinear::grid_dice(const int ru, const int rv)
{
	// Initialize grid and fill in the basics
	Grid *grid = new Grid(ru, rv, verts.size());

	// Fill in face and uvs
	grid->face_id = 0;
	grid->u1 = u_min;
	grid->v1 = v_min;
	grid->u2 = u_max;
	grid->v2 = v_min;
	grid->u3 = u_min;
	grid->v3 = v_max;
	grid->u4 = u_max;
	grid->v4 = v_max;

	// Generate verts
	Vec3 du1;
	Vec3 du2;
	Vec3 dv;
	Vec3 p1, p2, p3;
	int x, y;
	int i, time;
	// Dice for each time sample
	for (time = 0; time < verts.size(); time++) {
		// Deltas
		du1.x = (verts[time][1].x - verts[time][0].x) / (ru-1);
		du1.y = (verts[time][1].y - verts[time][0].y) / (ru-1);
		du1.z = (verts[time][1].z - verts[time][0].z) / (ru-1);

		du2.x = (verts[time][2].x - verts[time][3].x) / (ru-1);
		du2.y = (verts[time][2].y - verts[time][3].y) / (ru-1);
		du2.z = (verts[time][2].z - verts[time][3].z) / (ru-1);

		// Starting points
		p1 = verts[time][0];

		p2 = verts[time][3];

		// Walk along u
		for (x=0; x < ru; x++) {
			// Deltas
			dv = (p2 - p1) / (rv-1);

			// Starting point
			p3 = p1;

			// Walk along v
			for (y=0; y < rv; y++) {
				// Set grid vertex coordinates
				i = (ru*y+x) * grid->time_count + time;
				grid->verts[i] = p3;

				// Update point
				p3 = p3 + dv;
			}

			// Update points
			p1 = p1 + du1;
			p2 = p2 + du2;
		}
	}

	return grid;
}
