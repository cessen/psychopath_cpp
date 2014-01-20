#include "numtype.h"

#include <algorithm>
#include <iostream>
#include <limits>
#include <stdlib.h>
#include "bicubic.hpp"
#include "grid.hpp"
#include "config.hpp"
#include "global.hpp"

Bicubic::Bicubic(Vec3 v1,  Vec3 v2,  Vec3 v3,  Vec3 v4,
                 Vec3 v5,  Vec3 v6,  Vec3 v7,  Vec3 v8,
                 Vec3 v9,  Vec3 v10, Vec3 v11, Vec3 v12,
                 Vec3 v13, Vec3 v14, Vec3 v15, Vec3 v16)
{
	verts.resize(1);
	time_samples = 1;

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

	u_min = v_min = 0.0f;
	u_max = v_max = 1.0f;
}

void Bicubic::add_time_sample(Vec3 v1,  Vec3 v2,  Vec3 v3,  Vec3 v4,
                              Vec3 v5,  Vec3 v6,  Vec3 v7,  Vec3 v8,
                              Vec3 v9,  Vec3 v10, Vec3 v11, Vec3 v12,
                              Vec3 v13, Vec3 v14, Vec3 v15, Vec3 v16)
{
	const auto i = verts.size();
	verts.resize(verts.size()+1);
	++time_samples;

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

void Bicubic::finalize()
{
	// Calculate longest u-side of the patch
	for (int r = 0; r < 4; ++r) {
		float l = 0.0f;
		for (int c = 1; c < 4; ++c) {
			l += (verts[0][(r*4)+c] - verts[0][(r*4)+c-1]).length();
		}
		longest_u = l > longest_u ? l : longest_u;
	}

	// Calculate longest v-side of the patch
	for (int c = 0; c < 4; ++c) {
		float l = 0.0f;
		for (int r = 1; r < 4; ++r) {
			l += (verts[0][(r*4)+c] - verts[0][((r-1)*4)+c]).length();
		}
		longest_v = l > longest_v ? l : longest_v;
	}

	// Calculate bounds
	bbox.init(time_samples);
	for (int time = 0; time < time_samples; time++) {
		for (int i = 0; i < 16; i++) {
			bbox[time].min = min(bbox[time].min, verts[time][i]);
			bbox[time].max = max(bbox[time].max, verts[time][i]);
		}

		// Extend bounds for displacements
		for (int i = 0; i < 3; i++) {
			bbox[time].min[i] -= Config::displace_distance;
			bbox[time].max[i] += Config::displace_distance;
		}
	}
}


//////////////////////////////////////////////////////////////

size_t Bicubic::subdiv_estimate(float width) const
{
	if (width < Config::min_upoly_size)
		width = Config::min_upoly_size;

	// Power-of-two dicing rate
	size_t rate = (std::max(longest_u, longest_v) / (width * Config::dice_rate)) + 1;
	rate = intlog2(upper_power_of_two(rate));

	return rate;
}


BBoxT &Bicubic::bounds()
{
	return bbox;
}


int Bicubic::split(std::unique_ptr<DiceableSurfacePrimitive> primitives[])
{
	auto patch1 = new Bicubic();
	auto patch2 = new Bicubic();

	// Split
	if (longest_u > longest_v) {
		// Split on U
		for (int time=0; time < time_samples; time++) {
			// Calculate split geometry
			Vec3 verts1[4][4];
			Vec3 verts2[4][4];
			for (int r = 0; r < 4; ++r) {
				const auto rr = (r * 4);
				Vec3 tmp = (verts[time][rr+1] + verts[time][rr+2]) * 0.5;

				verts1[r][0] = verts[time][rr+0];
				verts1[r][1] = (verts[time][rr+0] + verts[time][rr+1]) * 0.5;
				verts1[r][2] = (tmp + verts1[r][1]) * 0.5;

				verts2[r][3] = verts[time][rr+3];
				verts2[r][2] = (verts[time][rr+3] + verts[time][rr+2]) * 0.5;
				verts2[r][1] = (tmp + verts2[r][2]) * 0.5;

				verts1[r][3] = (verts1[r][2] + verts2[r][1]) * 0.5;
				verts2[r][0] = verts1[r][3];
			}

			// Add split geometry to the new patches
			patch1->add_time_sample(verts1[0][0],
			                        verts1[0][1],
			                        verts1[0][2],
			                        verts1[0][3],

			                        verts1[1][0],
			                        verts1[1][1],
			                        verts1[1][2],
			                        verts1[1][3],

			                        verts1[2][0],
			                        verts1[2][1],
			                        verts1[2][2],
			                        verts1[2][3],

			                        verts1[3][0],
			                        verts1[3][1],
			                        verts1[3][2],
			                        verts1[3][3]
			                       );
			patch2->add_time_sample(verts2[0][0],
			                        verts2[0][1],
			                        verts2[0][2],
			                        verts2[0][3],

			                        verts2[1][0],
			                        verts2[1][1],
			                        verts2[1][2],
			                        verts2[1][3],

			                        verts2[2][0],
			                        verts2[2][1],
			                        verts2[2][2],
			                        verts2[2][3],

			                        verts2[3][0],
			                        verts2[3][1],
			                        verts2[3][2],
			                        verts2[3][3]
			                       );
		}

		// Fill in uv's
		patch1->u_min = u_min;
		patch1->u_max = (u_min + u_max) * 0.5;
		patch1->v_min = v_min;
		patch1->v_max = v_max;

		patch2->u_min = (u_min + u_max) * 0.5;
		patch2->u_max = u_max;
		patch2->v_min = v_min;
		patch2->v_max = v_max;
	} else {
		// Split on V
		for (int time=0; time < time_samples; time++) {
			// Calculate split geometry
			Vec3 verts1[4][4];
			Vec3 verts2[4][4];
			for (int c = 0; c < 4; ++c) {
				Vec3 tmp = (verts[time][c+(1*4)] + verts[time][c+(2*4)]) * 0.5;

				verts1[0][c] = verts[time][c+(0*4)];
				verts1[1][c] = (verts[time][c+(0*4)] + verts[time][c+(1*4)]) * 0.5;
				verts1[2][c] = (tmp + verts1[1][c]) * 0.5;

				verts2[3][c] = verts[time][c+(3*4)];
				verts2[2][c] = (verts[time][c+(3*4)] + verts[time][c+(2*4)]) * 0.5;
				verts2[1][c] = (tmp + verts2[2][c]) * 0.5;

				verts1[3][c] = (verts1[2][c] + verts2[1][c]) * 0.5;
				verts2[0][c] = verts1[3][c];
			}

			// Add split geometry to the new patches
			patch1->add_time_sample(verts1[0][0],
			                        verts1[0][1],
			                        verts1[0][2],
			                        verts1[0][3],

			                        verts1[1][0],
			                        verts1[1][1],
			                        verts1[1][2],
			                        verts1[1][3],

			                        verts1[2][0],
			                        verts1[2][1],
			                        verts1[2][2],
			                        verts1[2][3],

			                        verts1[3][0],
			                        verts1[3][1],
			                        verts1[3][2],
			                        verts1[3][3]
			                       );
			patch2->add_time_sample(verts2[0][0],
			                        verts2[0][1],
			                        verts2[0][2],
			                        verts2[0][3],

			                        verts2[1][0],
			                        verts2[1][1],
			                        verts2[1][2],
			                        verts2[1][3],

			                        verts2[2][0],
			                        verts2[2][1],
			                        verts2[2][2],
			                        verts2[2][3],

			                        verts2[3][0],
			                        verts2[3][1],
			                        verts2[3][2],
			                        verts2[3][3]
			                       );
		}

		// Fill in uv's
		patch1->u_min = u_min;
		patch1->u_max = u_max;
		patch1->v_min = v_min;
		patch1->v_max = (v_min + v_max) * 0.5;

		patch2->u_min = u_min;
		patch2->u_max = u_max;
		patch2->v_min = (v_min + v_max) * 0.5;
		patch2->v_max = v_max;
	}

	patch1->finalize();
	patch2->finalize();

	primitives[0] = std::unique_ptr<DiceableSurfacePrimitive>(patch1);
	primitives[1] = std::unique_ptr<DiceableSurfacePrimitive>(patch2);

	return 2;
}

std::unique_ptr<DiceableSurfacePrimitive> Bicubic::copy()
{
	auto patch = new Bicubic();

	// Copy verts
	patch->verts = verts;

	// Copy time sample count
	patch->time_samples = time_samples;

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


std::shared_ptr<MicroSurface> Bicubic::dice(size_t subdivisions)
{
	// Get dicing rate
	size_t u_rate = 1 << subdivisions;
	size_t v_rate = 1 << subdivisions;

	// Dice away!
	std::unique_ptr<Grid> grid = std::unique_ptr<Grid>(grid_dice(u_rate+1, v_rate+1));
	std::shared_ptr<MicroSurface> micro = std::make_shared<MicroSurface>(grid.get());

	return micro;
}

inline void eval_cubic_bezier_curve(int vert_count, int stride, Vec3 output[], Vec3 v0, Vec3 v1, Vec3 v2, Vec3 v3)
{
	const double dt = 1.0 / (vert_count - 1);

	// Differentials for x
	double dx0 = v0[0];
	double dx1 = (double(v1[0]) - v0[0]) * 3 * dt;
	double dx2 = ((double(v0[0]) * 6) - (double(v1[0]) * 12) + (double(v2[0]) * 6)) * dt * dt;
	double dx3 = ((double(v0[0]) * -6) + (double(v1[0]) * 18) - (double(v2[0]) * 18) + (double(v3[0]) * 6)) * dt * dt * dt;

	// Differentials for y
	double dy0 = v0[1];
	double dy1 = (double(v1[1]) - v0[1]) * 3 * dt;
	double dy2 = ((double(v0[1]) * 6) - (double(v1[1]) * 12) + (double(v2[1]) * 6)) * dt * dt;
	double dy3 = ((double(v0[1]) * -6) + (double(v1[1]) * 18) - (double(v2[1]) * 18) + (double(v3[1]) * 6)) * dt * dt * dt;

	// Differentials for z
	double dz0 = v0[2];
	double dz1 = (double(v1[2]) - v0[2]) * 3 * dt;
	double dz2 = ((double(v0[2]) * 6) - (double(v1[2]) * 12) + (double(v2[2]) * 6)) * dt * dt;
	double dz3 = ((double(v0[2]) * -6) + (double(v1[2]) * 18) - (double(v2[2]) * 18) + (double(v3[2]) * 6)) * dt * dt * dt;

	// To avoid divisions in inner loop
	const double inv_6 = 1.0 / 6.0;
	const double inv_2 = 1.0 / 2.0;

	// Go!
	output[0] = Vec3(dx0, dy0, dz0);
	for (int i = 1; i < vert_count; ++i) {
		// Find next point
		dx0 = dx0 + dx1 + (dx2 * inv_2) + (dx3 * inv_6);
		dy0 = dy0 + dy1 + (dy2 * inv_2) + (dy3 * inv_6);
		dz0 = dz0 + dz1 + (dz2 * inv_2) + (dx3 * inv_6);

		// Store point
		output[i*stride] = Vec3(dx0, dy0, dz0);

		// Update differentials
		dx1 += dx2 + (dx3 * inv_2);
		dx2 += dx3;
		dy1 += dy2 + (dy3 * inv_2);
		dy2 += dy3;
		dz1 += dz2 + (dz3 * inv_2);
		dz2 += dz3;
	}
}


/*
 * Dice the patch into a micropoly grid.
 * ru and rv are the resolution of the grid in vertices
 * in the u and v directions.
 */
Grid *Bicubic::grid_dice(const int ru, const int rv)
{
	// Initialize grid and fill in the basics
	Grid *grid = new Grid(ru, rv, time_samples);

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

	std::vector<Vec3> vs(rv*4); // Hold v-dicing before doing u-dicing

	// Dice for each time sample
	for (int time = 0; time < time_samples; time++) {
		// Dice along v-columns
		eval_cubic_bezier_curve(rv, 4, vs.data()+0, verts[time][0], verts[time][4], verts[time][8], verts[time][12]);
		eval_cubic_bezier_curve(rv, 4, vs.data()+1, verts[time][1], verts[time][5], verts[time][9], verts[time][13]);
		eval_cubic_bezier_curve(rv, 4, vs.data()+2, verts[time][2], verts[time][6], verts[time][10], verts[time][14]);
		eval_cubic_bezier_curve(rv, 4, vs.data()+3, verts[time][3], verts[time][7], verts[time][11], verts[time][15]);

		//i = (ru*y+x) * grid->time_count + time;
		// Dice along u-rows
		for (int v = 0; v < rv; ++v) {
			eval_cubic_bezier_curve(ru, grid->time_count, grid->verts.data()+(ru*v*grid->time_count)+time,
			                        vs[(v*4)],
			                        vs[(v*4)+1],
			                        vs[(v*4)+2],
			                        vs[(v*4)+3]);
		}
	}

	return grid;
}
