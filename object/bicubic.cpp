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
#include "grid.hpp"
#include "config.hpp"
#include "global.hpp"

#include "surface_closure.hpp"
#include "closure_union.hpp"


__attribute__((always_inline))
inline void split_u(const Vec3 p[], Vec3 p1[], Vec3 p2[])
{
	for (int r = 0; r < 4; ++r) {
		const auto rr = (r * 4);
		Vec3 tmp = (p[rr+1] + p[rr+2]) * 0.5;

		p2[rr+3] = p[rr+3];
		p2[rr+2] = (p[rr+3] + p[rr+2]) * 0.5;
		p2[rr+1] = (tmp + p2[rr+2]) * 0.5;

		p1[rr+0] = p[rr+0];
		p1[rr+1] = (p[rr+0] + p[rr+1]) * 0.5;
		p1[rr+2] = (tmp + p1[rr+1]) * 0.5;

		p1[rr+3] = (p1[rr+2] + p2[rr+1]) * 0.5;
		p2[rr+0] = p1[rr+3];
	}
}


__attribute__((always_inline))
inline void split_v(const Vec3 p[], Vec3 p1[], Vec3 p2[])
{
	for (int c = 0; c < 4; ++c) {
		Vec3 tmp = (p[c+(1*4)] + p[c+(2*4)]) * 0.5;

		p2[c+(3*4)] = p[c+(3*4)];
		p2[c+(2*4)] = (p[c+(3*4)] + p[c+(2*4)]) * 0.5;
		p2[c+(1*4)] = (tmp + p2[c+(2*4)]) * 0.5;

		p1[c+(0*4)] = p[c+(0*4)];
		p1[c+(1*4)] = (p[c+(0*4)] + p[c+(1*4)]) * 0.5;
		p1[c+(2*4)] = (tmp + p1[c+(1*4)]) * 0.5;

		p1[c+(3*4)] = (p1[c+(2*4)] + p2[c+(1*4)]) * 0.5;
		p2[c+(0*4)] = p1[c+(3*4)];
	}
}


inline Vec3 dp_u(const Vec3 p[], float u, float v)
{

	// First we interpolate across v to get a curve
	const float iv = 1.0f - v;
	const float b0 = iv * iv * iv;
	const float b1 = 3.0f * v * iv * iv;
	const float b2 = 3.0f * v * v * iv;
	const float b3 = v * v * v;
	Vec3 c[4];
	c[0] = (p[0] * b0) + (p[4] * b1) + (p[8] * b2) + (p[12] * b3);
	c[1] = (p[1] * b0) + (p[5] * b1) + (p[9] * b2) + (p[13] * b3);
	c[2] = (p[2] * b0) + (p[6] * b1) + (p[10] * b2) + (p[14] * b3);
	c[3] = (p[3] * b0) + (p[7] * b1) + (p[11] * b2) + (p[15] * b3);

	// Now we use the derivatives across u to find dp
	const float iu = 1.0f - u;
	const float d0 = -3.0f * iu * iu;
	const float d1 = (3.0f * iu * iu) - (6.0f * iu * u);
	const float d2 = (6.0f * iu * u) - (3.0f * u * u);
	const float d3 = 3.0f * u * u;

	return (c[0] * d0) + (c[1] * d1) + (c[2] * d2) + (c[3] * d3);
}


inline Vec3 dp_v(const Vec3 p[], float u, float v)
{

	// First we interpolate across u to get a curve
	const float iu = 1.0f - u; // We use this a lot, so pre-calculate
	const float b0 = iu * iu * iu;
	const float b1 = 3.0f * u * iu * iu;
	const float b2 = 3.0f * u * u * iu;
	const float b3 = u * u * u;
	Vec3 c[4];
	c[0] = (p[0] * b0) + (p[1] * b1) + (p[2] * b2) + (p[3] * b3);
	c[1] = (p[4] * b0) + (p[5] * b1) + (p[6] * b2) + (p[7] * b3);
	c[2] = (p[8] * b0) + (p[9] * b1) + (p[10] * b2) + (p[11] * b3);
	c[3] = (p[12] * b0) + (p[13] * b1) + (p[14] * b2) + (p[15] * b3);

	// Now we use the derivatives across u to find dp
	const float iv = 1.0f - v; // We use this a lot, so pre-calculate
	const float d0 = -3.0f * iv * iv;
	const float d1 = (3.0f * iv * iv) - (6.0f * iv * v);
	const float d2 = (6.0f * iv * v) - (3.0f * v * v);
	const float d3 = 3.0f * v * v;

	return (c[0] * d0) + (c[1] * d1) + (c[2] * d2) + (c[3] * d3);
}


inline BBox bound(const std::array<Vec3, 16>& p)
{
	BBox bb = BBox(p[0], p[0]);

	for (int i = 1; i < 16; ++i) {
		bb.min = min(bb.min, p[i]);
		bb.max = max(bb.max, p[i]);
	}

	return bb;
}

///////////////////////////////////////////////////



Bicubic::Bicubic(Vec3 v1,  Vec3 v2,  Vec3 v3,  Vec3 v4,
                 Vec3 v5,  Vec3 v6,  Vec3 v7,  Vec3 v8,
                 Vec3 v9,  Vec3 v10, Vec3 v11, Vec3 v12,
                 Vec3 v13, Vec3 v14, Vec3 v15, Vec3 v16)
{
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
	longest_u = 0.0f;
	for (int r = 1; r < 2; ++r) {
		float l = 0.0f;
		for (int c = 1; c < 4; ++c) {
			l += longest_axis(verts[0][(r*4)+c] - verts[0][(r*4)+c-1]);
		}
		longest_u = std::max(l, longest_u);
	}

	// Calculate longest v-side of the patch
	longest_v = 0.0f;
	for (int c = 1; c < 2; ++c) {
		float l = 0.0f;
		for (int r = 1; r < 4; ++r) {
			l += longest_axis(verts[0][(r*4)+c] - verts[0][((r-1)*4)+c]);
		}
		longest_v = std::max(l, longest_v);
	}

	// Calculate log-base-2 of the widest part of the patch
	log_widest = fastlog2(std::max(longest_u, longest_v));

	// Calculate bounds
	bbox.resize(verts.size());
	for (size_t time = 0; time < verts.size(); time++) {
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
const std::vector<BBox> &Bicubic::bounds() const
{
	return bbox;
}


#if 0
size_t Bicubic::subdiv_estimate(float width) const
{
	// Since we want to end up with the log-base-2 of
	// the division anyway, we just do the log first and
	// subtract.  Using a very approximate log2, but in
	// practice it works very well.
	const float rate = log_widest - fasterlog2(width * Config::dice_rate) + 1.0f;
	return std::max(rate, 0.0f);
}


int Bicubic::split(std::unique_ptr<DiceableSurface> objects[])
{
	auto patch1 = new Bicubic();
	auto patch2 = new Bicubic();

	patch1->verts.resize(verts.size());
	patch2->verts.resize(verts.size());

	// Split
	if (longest_u > longest_v) {
		// Split on U
		for (size_t time=0; time < verts.size(); time++) {
			// Calculate split geometry
			split_u(&(verts[time][0]), &(patch1->verts[time][0]), &(patch2->verts[time][0]));
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
		for (size_t time=0; time < verts.size(); time++) {
			// Calculate split geometry
			split_v(&(verts[time][0]), &(patch1->verts[time][0]), &(patch2->verts[time][0]));
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

	objects[0] = std::unique_ptr<DiceableSurface>(patch1);
	objects[1] = std::unique_ptr<DiceableSurface>(patch2);

	return 2;
}

std::unique_ptr<DiceableSurface> Bicubic::copy() const
{
	auto patch = new Bicubic();

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
	patch->log_widest = log_widest;

	return std::unique_ptr<DiceableSurface>(patch);
}


std::shared_ptr<MicroSurface> Bicubic::dice(size_t subdivisions) const
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
Grid *Bicubic::grid_dice(const int ru, const int rv) const
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

	std::vector<Vec3> vs(rv*4); // Hold v-dicing before doing u-dicing

	// Dice for each time sample
	for (size_t time = 0; time < verts.size(); time++) {
		// Dice along v-columns
		eval_cubic_bezier_curve(rv, 4, vs.data()+0, verts[time][0], verts[time][4], verts[time][8], verts[time][12]);
		eval_cubic_bezier_curve(rv, 4, vs.data()+1, verts[time][1], verts[time][5], verts[time][9], verts[time][13]);
		eval_cubic_bezier_curve(rv, 4, vs.data()+2, verts[time][2], verts[time][6], verts[time][10], verts[time][14]);
		eval_cubic_bezier_curve(rv, 4, vs.data()+3, verts[time][3], verts[time][7], verts[time][11], verts[time][15]);

		//i = (ru*y+x) * grid->time_count + time;
		// Dice along u-rows
		for (int v = 0; v < rv; ++v) {
			eval_cubic_bezier_curve(ru, 1, grid->verts.data()+(ru*v)+(time*ru*rv),
			                        vs[(v*4)],
			                        vs[(v*4)+1],
			                        vs[(v*4)+2],
			                        vs[(v*4)+3]);
		}
	}

	return grid;
}
#endif


bool Bicubic::intersect_single_ray_helper(const Ray &ray, const std::array<Vec3, 16> &patch, const std::array<Vec3, 16> &subpatch, const std::tuple<float, float, float, float> &uvs, Intersection intersections[])
{
#define STACK_SIZE 64
	int stack_i = 0;
	std::array<Vec3, 16> patch_stack[STACK_SIZE];
	std::tuple<float, float, float, float> uv_stack[STACK_SIZE]; // (min_u, max_u, min_v, max_v)
	std::pair<float, float> hit_stack[STACK_SIZE]; // (hitt0, hitt1)

	// Hit data
	bool hit = false;
	float t = ray.max_t;
	float u = 0.0f;
	float v = 0.0f;
	float offset = 0.0f;

	float hitt0, hitt1;

	// Initialize stacks
	// TODO: take into account ray time
	patch_stack[0] = subpatch;
	uv_stack[0] = uvs;
	hit_stack[0] = std::make_pair(0.0f, 0.0f);

	// Test root patch for intersection before iteration
	if (bounds()[0].intersect_ray(ray, &hitt0, &hitt1, t)) {
		hit_stack[0].first = (hitt0 + hitt1) * 0.5f;
		hit_stack[0].second = longest_axis(bounds()[0].max - bounds()[0].min);
	} else {
		return false;
	}

	// Iterate down to find an intersection
	while (stack_i >= 0) {
		const float tt = hit_stack[stack_i].first;
		const float max_dim = hit_stack[stack_i].second;

		// LEAF
		if (max_dim <= Config::min_upoly_size || stack_i == (STACK_SIZE-1)) {
			if (tt > 0.0f && tt <= t) {
				if ((ray.flags() & Ray::IS_OCCLUSION) != 0)
					return true;

				hit = true;
				t = tt;
				u = (std::get<0>(uv_stack[stack_i]) + std::get<1>(uv_stack[stack_i])) * 0.5f;
				v = (std::get<2>(uv_stack[stack_i]) + std::get<3>(uv_stack[stack_i])) * 0.5f;
				offset = max_dim * 1.74f;
			}

			--stack_i;
		}
		// INNER, do split
		else {
			//auto patch = patch_stack[stack_i];
			auto uv = uv_stack[stack_i];

			const float ulen = longest_axis(patch_stack[stack_i][0] - patch_stack[stack_i][3]);
			const float vlen = longest_axis(patch_stack[stack_i][0] - patch_stack[stack_i][4*3]);

			// Split U
			if (ulen > vlen) {
				split_u(&(patch_stack[stack_i][0]), &(patch_stack[stack_i][0]), &(patch_stack[stack_i+1][0]));

				// Fill in uv's
				std::get<0>(uv_stack[stack_i]) = std::get<0>(uv);
				std::get<1>(uv_stack[stack_i]) = (std::get<0>(uv) + std::get<1>(uv)) * 0.5;
				std::get<2>(uv_stack[stack_i]) = std::get<2>(uv);
				std::get<3>(uv_stack[stack_i]) = std::get<3>(uv);

				std::get<0>(uv_stack[stack_i+1]) = (std::get<0>(uv) + std::get<1>(uv)) * 0.5;
				std::get<1>(uv_stack[stack_i+1]) = std::get<1>(uv);
				std::get<2>(uv_stack[stack_i+1]) = std::get<2>(uv);
				std::get<3>(uv_stack[stack_i+1]) = std::get<3>(uv);

			}
			// Split V
			else {
				split_v(&(patch_stack[stack_i][0]), &(patch_stack[stack_i][0]), &(patch_stack[stack_i+1][0]));

				// Fill in uv's
				std::get<0>(uv_stack[stack_i]) = std::get<0>(uv);
				std::get<1>(uv_stack[stack_i]) = std::get<1>(uv);
				std::get<2>(uv_stack[stack_i]) = std::get<2>(uv);
				std::get<3>(uv_stack[stack_i]) = (std::get<2>(uv) + std::get<3>(uv)) * 0.5;

				std::get<0>(uv_stack[stack_i+1]) = std::get<0>(uv);
				std::get<1>(uv_stack[stack_i+1]) = std::get<1>(uv);
				std::get<2>(uv_stack[stack_i+1]) = (std::get<2>(uv) + std::get<3>(uv)) * 0.5;
				std::get<3>(uv_stack[stack_i+1]) = std::get<3>(uv);
			}

			// Test ray against their bounds
			float hitta0, hitta1;
			float hittb0, hittb1;
			const auto bb1 = bound(patch_stack[stack_i]);
			const auto bb2 = bound(patch_stack[stack_i+1]);
			const bool hita = bb1.intersect_ray(ray, &hitta0, &hitta1, t);
			const bool hitb = bb2.intersect_ray(ray, &hittb0, &hittb1, t);
			hit_stack[stack_i].first = (hitta0 + hitta1) * 0.5f;
			hit_stack[stack_i].second = longest_axis(bb1.max - bb1.min);
			hit_stack[stack_i+1].first = (hittb0 + hittb1) * 0.5f;
			hit_stack[stack_i+1].second = longest_axis(bb2.max - bb2.min);

			// Set up the stack properly depending on results
			if (hita && hitb) {
				if (hitta0 < hittb0) {
					std::swap(patch_stack[stack_i], patch_stack[stack_i+1]);
					std::swap(uv_stack[stack_i], uv_stack[stack_i+1]);
					std::swap(hit_stack[stack_i], hit_stack[stack_i+1]);
				}
				++stack_i;
			} else if (hitb) {
				patch_stack[stack_i] = patch_stack[stack_i+1];
				uv_stack[stack_i] = uv_stack[stack_i+1];
				hit_stack[stack_i] = hit_stack[stack_i+1];
			} else if (hita) {
				// Do nothing
			} else {
				--stack_i;
			}
		}
	}


	// Fill in intersection data, if needed
	if (hit && (ray.flags() & Ray::IS_OCCLUSION) == 0) {
		auto intersection = &(intersections[ray.id]);

		intersection->t = t;

		intersection->geo.p = ray.o + (ray.d * t);
		intersection->geo.u = u;
		intersection->geo.v = v;

		// Differential position
		intersection->geo.dpdu = dp_u(&(patch[0]), u, v);
		intersection->geo.dpdv = dp_v(&(patch[0]), u, v);

		// Surface normal
		intersection->geo.n = cross(intersection->geo.dpdv, intersection->geo.dpdu).normalized();

		// Did te ray hit from the back-side of the surface?
		intersection->backfacing = dot(intersection->geo.n, ray.d.normalized()) > 0.0f;

		// Differential normal
		// TODO
		intersection->geo.dndu = Vec3(1.0f, 0.0f, 0.0f);
		intersection->geo.dndv = Vec3(0.0f, 1.0f, 0.0f);

		intersection->offset = intersection->geo.n * offset;
	}

	return hit;
}


void Bicubic::intersect_rays(const std::vector<Transform>& parent_xforms, Ray* ray_begin, Ray* ray_end, Intersection *intersections, Stack* data_stack)
{
#define STACK_SIZE 64
	const size_t tsc = verts.size(); // Time sample count
	int stack_i = 0;
	std::pair<Ray*, Ray*> ray_stack[STACK_SIZE];
	BBox* bboxes = data_stack->push_frame<BBox>(tsc).first;
	Stack &patch_stack = *data_stack;
	std::tuple<float, float, float, float> uv_stack[STACK_SIZE]; // (min_u, max_u, min_v, max_v)

	// Initialize stacks
	// TODO: take into account ray time
	ray_stack[0] = std::make_pair(ray_begin, ray_end);
	auto tmp = patch_stack.push_frame<std::array<Vec3, 16>>(tsc).first;
	for (int i = 0; i < tsc; ++i) {
		tmp[i] = verts[i];
	}
	uv_stack[0] = std::tuple<float, float, float, float>(u_min, u_max, v_min, v_max);

	// Iterate down to find an intersection
	while (stack_i >= 0) {
		auto cur_patches = patch_stack.top_frame<std::array<Vec3, 16>>().first;

		// Calculate bounding boxes and max_dim
		bboxes[0] = bound(cur_patches[0]);
		float max_dim = longest_axis(bboxes[0].max - bboxes[0].min);
		for (int i = 1; i < tsc; ++i) {
			bboxes[i] = bound(cur_patches[i]);
			max_dim = std::max(max_dim, longest_axis(bboxes[i].max - bboxes[i].min));
		}

		// TEST RAYS AGAINST BBOX
		ray_stack[stack_i].first = mutable_partition(ray_stack[stack_i].first, ray_stack[stack_i].second, [&](Ray& ray) {
			if ((ray.flags() & Ray::DONE) != 0) {
				return true;
			}

			float hitt0, hitt1;
			if (bboxes[0].intersect_ray(ray, &hitt0, &hitt1, ray.max_t)) {
				const float width = ray.min_width(hitt0, hitt1) * Config::dice_rate;
				// LEAF, so we don't have to go deeper, regardless of whether
				// we hit it or not.
				if (max_dim <= width || stack_i == (STACK_SIZE-1)) {
					const float tt = (hitt0 + hitt1) * 0.5f;
					if (tt > 0.0f && tt <= ray.max_t) {
						auto &inter = intersections[ray.id];
						inter.hit = true;
						if ((ray.flags() & Ray::IS_OCCLUSION) != 0) {
							ray.flags() |= Ray::DONE;
						} else {
							ray.max_t = tt;

							const float u = (std::get<0>(uv_stack[stack_i]) + std::get<1>(uv_stack[stack_i])) * 0.5f;
							const float v = (std::get<2>(uv_stack[stack_i]) + std::get<3>(uv_stack[stack_i])) * 0.5f;
							const float offset = max_dim * 1.74f;

							inter.t = tt;

							inter.space = parent_xforms.size() > 0 ? lerp_seq(ray.time, parent_xforms) : Transform();
							//inter.surface_closure.init(GTRClosure(Color(0.9, 0.9, 0.9), 0.02f, 1.2f, 0.25f));
							//inter.surface_closure.init(GTRClosure(Color(0.9, 0.9, 0.9), 0.0f, 1.5f, 0.25f));
							inter.surface_closure.init(LambertClosure(Color(0.9, 0.9, 0.9)));

							inter.geo.p = ray.o + (ray.d * tt);
							inter.geo.u = u;
							inter.geo.v = v;

							// Differential position
							inter.geo.dpdu = dp_u(&(verts[0][0]), u, v);
							inter.geo.dpdv = dp_v(&(verts[0][0]), u, v);

							// Surface normal
							inter.geo.n = cross(inter.geo.dpdv, inter.geo.dpdu).normalized();

							// Did te ray hit from the back-side of the surface?
							inter.backfacing = dot(inter.geo.n, ray.d.normalized()) > 0.0f;

							// Differential normal
							// TODO
							inter.geo.dndu = Vec3(0.0f, 0.0f, 0.0f);
							inter.geo.dndv = Vec3(0.0f, 0.0f, 0.0f);

							inter.offset = inter.geo.n * offset;
						}
					}

					return true;
				}
				// INNER, so we have to go deeper
				else {
					return false;
				}
			} else {
				// Didn't hit it, so we don't have to go deeper
				return true;
			}
		});
		// END TEST RAYS AGAINST BBOX

		// Split patch for further traversal if necessary
		if (ray_stack[stack_i].first != ray_stack[stack_i].second) {
			auto uv = uv_stack[stack_i];
			auto next_patches = patch_stack.push_frame<std::array<Vec3, 16>>(tsc).first;

			const float ulen = longest_axis(cur_patches[0][0] - cur_patches[0][3]);
			const float vlen = longest_axis(cur_patches[0][0] - cur_patches[0][4*3]);

			// Split U
			if (ulen > vlen) {
				for (int i = 0; i < tsc; ++i) {
					split_u(&(cur_patches[i][0]), &(cur_patches[i][0]), &(next_patches[i][0]));
				}

				// Fill in uv's
				std::get<0>(uv_stack[stack_i]) = std::get<0>(uv);
				std::get<1>(uv_stack[stack_i]) = (std::get<0>(uv) + std::get<1>(uv)) * 0.5;
				std::get<2>(uv_stack[stack_i]) = std::get<2>(uv);
				std::get<3>(uv_stack[stack_i]) = std::get<3>(uv);

				std::get<0>(uv_stack[stack_i+1]) = (std::get<0>(uv) + std::get<1>(uv)) * 0.5;
				std::get<1>(uv_stack[stack_i+1]) = std::get<1>(uv);
				std::get<2>(uv_stack[stack_i+1]) = std::get<2>(uv);
				std::get<3>(uv_stack[stack_i+1]) = std::get<3>(uv);

			}
			// Split V
			else {
				for (int i = 0; i < tsc; ++i) {
					split_v(&(cur_patches[i][0]), &(cur_patches[i][0]), &(next_patches[i][0]));
				}

				// Fill in uv's
				std::get<0>(uv_stack[stack_i]) = std::get<0>(uv);
				std::get<1>(uv_stack[stack_i]) = std::get<1>(uv);
				std::get<2>(uv_stack[stack_i]) = std::get<2>(uv);
				std::get<3>(uv_stack[stack_i]) = (std::get<2>(uv) + std::get<3>(uv)) * 0.5;

				std::get<0>(uv_stack[stack_i+1]) = std::get<0>(uv);
				std::get<1>(uv_stack[stack_i+1]) = std::get<1>(uv);
				std::get<2>(uv_stack[stack_i+1]) = (std::get<2>(uv) + std::get<3>(uv)) * 0.5;
				std::get<3>(uv_stack[stack_i+1]) = std::get<3>(uv);
			}

			ray_stack[stack_i + 1] = ray_stack[stack_i];

			++stack_i;
		} else {
			--stack_i;
			patch_stack.pop_frame();
		}
	}

	patch_stack.pop_frame(); // Pop BBoxes
}