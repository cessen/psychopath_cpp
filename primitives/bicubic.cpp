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
	verts.resize(16);
	time_samples = 1;

	verts[0]  = v1;
	verts[1]  = v2;
	verts[2]  = v3;
	verts[3]  = v4;

	verts[4]  = v5;
	verts[5]  = v6;
	verts[6]  = v7;
	verts[7]  = v8;

	verts[8]  = v9;
	verts[9]  = v10;
	verts[10] = v11;
	verts[11] = v12;

	verts[12] = v13;
	verts[13] = v14;
	verts[14] = v15;
	verts[15] = v16;

	u_min = v_min = 0.0f;
	u_max = v_max = 1.0f;
}

void Bicubic::add_time_sample(Vec3 v1,  Vec3 v2,  Vec3 v3,  Vec3 v4,
                              Vec3 v5,  Vec3 v6,  Vec3 v7,  Vec3 v8,
                              Vec3 v9,  Vec3 v10, Vec3 v11, Vec3 v12,
                              Vec3 v13, Vec3 v14, Vec3 v15, Vec3 v16)
{
	const auto i = verts.size();
	verts.resize(verts.size()+16);
	++time_samples;

	verts[i]  = v1;
	verts[i+1]  = v2;
	verts[i+2]  = v3;
	verts[i+3]  = v4;

	verts[i+4]  = v5;
	verts[i+5]  = v6;
	verts[i+6]  = v7;
	verts[i+7]  = v8;

	verts[i+8]  = v9;
	verts[i+9]  = v10;
	verts[i+10] = v11;
	verts[i+11] = v12;

	verts[i+12] = v13;
	verts[i+13] = v14;
	verts[i+14] = v15;
	verts[i+15] = v16;
}

void Bicubic::finalize()
{
	// Calculate longest u-side of the patch
	for (int i = 0; i < 4; ++i) {
		float l = 0.0f;
		for (int ui = 1; ui < 4; ++i) {
			l += (verts[(i*4)+ui] - verts[(i*4)+ui-1]).length();
		}
		longest_u = l < longest_u ? l : longest_u;
	}

	// Calculate longest v-side of the patch
	float longest_v = 0.0f;
	for (int i = 0; i < 4; ++i) {
		float l = 0.0f;
		for (int vi = 1; vi < 4; ++i) {
			l += (verts[(vi*4)+i] - verts[((vi-1)*4)+i]).length();
		}
		longest_v = l < longest_v ? l : longest_v;
	}

	// Calculate bounds
	bbox.init(time_samples);
	for (int time = 0; time < time_samples; time++) {
		for (int i = 0; i < 16; i++) {
			bbox[time].min = min(bbox[time].min, verts[(time*16)+i]);
			bbox[time].max = max(bbox[time].max, verts[(time*16)+i]);
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


//////////////////////////////////////////////////////////////

size_t Bicubic::subdiv_estimate(float width) const
{
	if (width < Config::min_upoly_size)
		width = Config::min_upoly_size;

	// Power-of-two dicing rate
	size_t rate = (std::max(longest_u, longest_v) / (width * Config::dice_rate)) + 1;
	rate = upper_power_of_two(rate);
	return intlog2(rate);
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
		for (int i=0; i < time_samples; i++) {
			const auto ii = i * 16;

			// Calculate split geometry
			Vec3 verts1[4][4];
			Vec3 verts2[4][4];
			for (int r = 0; r < 4; ++r) {
				const auto rr = ii + (r * 4);
				Vec3 tmp = (verts[rr+1] + verts[rr+2]) * 0.5;

				verts1[r][0] = verts[rr+0];
				verts1[r][1] = (verts[rr+0] + verts[rr+1]) * 0.5;
				verts1[r][2] = (tmp + verts1[r][1]) * 0.5;

				verts2[r][3] = verts[rr+3];
				verts2[r][2] = (verts[rr+3] + verts[rr+2]) * 0.5;
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
		for (int i=0; i < time_samples; i++) {
			const auto ii = i * 16;

			// Calculate split geometry
			Vec3 verts1[4][4];
			Vec3 verts2[4][4];
			for (int c = 0; c < 4; ++c) {
				const auto cc = ii + c;
				Vec3 tmp = (verts[cc+(1*4)] + verts[cc+(2*4)]) * 0.5;

				verts1[0][c] = verts[cc+(0*4)];
				verts1[1][c] = (verts[cc+(0*4)] + verts[cc+(1*4)]) * 0.5;
				verts1[2][c] = (tmp + verts1[1][c]) * 0.5;

				verts2[3][c] = verts[cc+(3*4)];
				verts2[2][c] = (verts[cc+(3*4)] + verts[cc+(2*4)]) * 0.5;
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
	Grid *grid = grid_dice(u_rate+1, v_rate+1);
	std::shared_ptr<MicroSurface> micro = std::make_shared<MicroSurface>(grid);

	delete grid;

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
	for (int i = 1; i < (vert_count*stride); i += stride) {
		// Find next point
		dx0 = dx0 + dx1 + (dx2 * inv_2) + (dx3 * inv_6);
		dy0 = dy0 + dy1 + (dy2 * inv_2) + (dy3 * inv_6);
		dz0 = dz0 + dz1 + (dz2 * inv_2) + (dx3 * inv_6);

		// Store point
		output[i] = Vec3(dx0, dy0, dz0);

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
	const int tot_verts = ru * rv;

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

	// Dice for each time sample
	for (int time = 0; time < time_samples; time++) {
		// Populate the first four v-columns with the diced points
		// for working on the u-rows
		eval_cubic_bezier_curve(rv, ru, grid->verts.data()+(time*tot_verts)+0, verts[(time*16)+0], verts[(time*16)+4], verts[(time*16)+8], verts[(time*16)+12]);
		eval_cubic_bezier_curve(rv, ru, grid->verts.data()+(time*tot_verts)+1, verts[(time*16)+1], verts[(time*16)+5], verts[(time*16)+9], verts[(time*16)+13]);
		eval_cubic_bezier_curve(rv, ru, grid->verts.data()+(time*tot_verts)+2, verts[(time*16)+2], verts[(time*16)+6], verts[(time*16)+10], verts[(time*16)+14]);
		eval_cubic_bezier_curve(rv, ru, grid->verts.data()+(time*tot_verts)+3, verts[(time*16)+3], verts[(time*16)+7], verts[(time*16)+11], verts[(time*16)+15]);

		// Dice along u-rows
		for (int v = 0; v < rv; ++v) {
			eval_cubic_bezier_curve(ru, 1, grid->verts.data()+(time*tot_verts)+(v*ru), grid->verts[(time*tot_verts)+(v*ru)],
			                        grid->verts[(time*tot_verts)+(v*ru)+1],
			                        grid->verts[(time*tot_verts)+(v*ru)+2],
			                        grid->verts[(time*tot_verts)+(v*ru)+3]);
		}
	}

	return grid;
}
