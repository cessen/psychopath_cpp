#include "numtype.h"

#include "grid.hpp"


bool Grid::calc_normals(Vec3 *normals)
{
	Vec3 p(0,0,0);
	Vec3 vec[4] = {Vec3(0,0,0), Vec3(0,0,0), Vec3(0,0,0), Vec3(0,0,0)};
	bool v_avail[4] = {false, false, false, false};
	Vec3 n[4] = {Vec3(0,0,0), Vec3(0,0,0), Vec3(0,0,0), Vec3(0,0,0)};
	bool n_avail[4] = {false, false, false, false};

	uint_i upoly_i;
	uint_i i2, n_count = 0;

	for (uint_i time=0; time < time_count; time++) {
		for (int32 v=0; v < res_v; v++) {
			for (int32 u=0; u < res_u; u++) {
				upoly_i = (v * res_u) + u;

				// Get the center point
				p = verts[(upoly_i)*time_count+time];

				// Get the four vectors out from it (or whatever subset exist)
				if (u < (res_u-1)) {
					vec[0] = verts[(upoly_i + 1)*time_count+time] - p;
					v_avail[0] = true;
				}

				if (v < (res_v-1)) {
					vec[1] = verts[(upoly_i + res_u)*time_count+time] - p;
					v_avail[1] = true;
				}

				if (u > 0) {
					vec[2] = verts[(upoly_i - 1)*time_count+time] - p;
					v_avail[2] = true;
				}

				if (v > 0) {
					vec[3] = verts[(upoly_i - res_u)*time_count+time] - p;
					v_avail[3] = true;
				}

				// Calculate the normals
				for (uint_i i=0; i < 4; i++) {
					i2 = (i + 1) % 4;

					if (v_avail[i] && v_avail[i2]) {
						n[i] = cross(vec[i], vec[i2]);
						n_avail[i] = true;
						n_count++;
					}
				}

				// Average the normals
				normals[upoly_i*time_count+time] = Vec3(0,0,0);
				for (uint_i i=0; i < 4; i++) {
					if (n_avail[i])
						normals[upoly_i*time_count+time] = normals[upoly_i*time_count+time] + n[i];
				}
				normals[upoly_i*time_count+time] = normals[upoly_i*time_count+time] / (float32)(n_count);

				// Normalize the normal
				normals[upoly_i*time_count+time].normalize();
			}
		}
	}

	return true;
}


bool Grid::calc_uvs(float32 *uvs)
{
	// Abusing "x" and "y" names here as substitute for u and v,
	// to keep things clear.

	// Put the corner uv's into more convenient form
	Vec3 uv1 = Vec3(u1, v1, 0);
	Vec3 uv2 = Vec3(u2, v2, 0);
	Vec3 uv3 = Vec3(u3, v3, 0);
	Vec3 uv4 = Vec3(u4, v4, 0);

	// Calculate deltas along y
	const Vec3 uv_dy1 = (uv3 - uv1) / res_v;
	const Vec3 uv_dy2 = (uv4 - uv2) / res_v;

	Vec3 uv_y1 = uv1;
	Vec3 uv_y2 = uv2;
	for (uint_i y = 0; y < res_v; y++) {
		const Vec3 uv_dx = (uv_y2 - uv_y1) / res_u; // Calculate delta along x
		Vec3 uv_x = uv_y1;
		for (uint_i x = 0; x < res_u; x++) {
			uvs[(y*res_u+x)*2] = uv_x.x;
			uvs[(y*res_u+x)*2+1] = uv_x.y;
			uv_x = uv_x + uv_dx;
		}
		uv_y1 = uv_y1 + uv_dy1;
		uv_y2 = uv_y2 + uv_dy2;
	}

	return true;
}
