#include "numtype.h"

#include <algorithm>
#include <iostream>
#include <limits>
#include <stdlib.h>
#include <cmath>
#include "bilinear.hpp"
#include "grid.hpp"
#include "config.hpp"
#include "global.hpp"


__attribute__((always_inline))
inline void split_u(const Vec3 p[], Vec3 p1[], Vec3 p2[])
{
	p2[0] = (p[0] + p[1]) * 0.5;
	p2[1] = p[1];
	p2[2] = p[2];
	p2[3] = (p[2] + p[3]) * 0.5;

	p1[0] = p[0];
	p1[1] = (p[0] + p[1]) * 0.5;
	p1[2] = (p[2] + p[3]) * 0.5;
	p1[3] = p[3];
}


__attribute__((always_inline))
inline void split_v(const Vec3 p[], Vec3 p1[], Vec3 p2[])
{
	p2[0] = (p[3] + p[0]) * 0.5;
	p2[1] = (p[1] + p[2]) * 0.5;
	p2[2] = p[2];
	p2[3] = p[3];

	p1[0] = p[0];
	p1[1] = p[1];
	p1[2] = (p[1] + p[2]) * 0.5;
	p1[3] = (p[3] + p[0]) * 0.5;
}


inline Vec3 dp_u(const Vec3 p[], float u, float v)
{
	// First we interpolate across v to get a curve
	const float iv = 1.0f - v;
	Vec3 c[2];
	c[0] = (p[0] * iv) + (p[3] * v);
	c[1] = (p[1] * iv) + (p[2] * v);

	// Now we use the derivatives across u to find dp
	return c[1] - c[0];
}


inline Vec3 dp_v(const Vec3 p[], float u, float v)
{

	// First we interpolate across u to get a curve
	const float iu = 1.0f - u; // We use this a lot, so pre-calculate
	Vec3 c[2];
	c[0] = (p[0] * iu) + (p[1] * v);
	c[1] = (p[3] * iu) + (p[2] * v);;

	// Now we use the derivatives across u to find dp
	return c[1] - c[0];
}


inline BBox bound(const std::array<Vec3, 4>& p)
{
	BBox bb = BBox(p[0], p[0]);;

	for (int i = 1; i < 4; ++i) {
		bb.min = min(bb.min, p[i]);
		bb.max = max(bb.max, p[i]);
	}

	return bb;
}


Bilinear::Bilinear(Vec3 v1, Vec3 v2, Vec3 v3, Vec3 v4)
{
	verts.push_back( {{v1,v2,v3,v4}});
}

void Bilinear::finalize()
{
	// Calculate longest sides of the patch
	longest_u = std::max((verts[0][0] - verts[0][1]).length(), (verts[0][2] - verts[0][3]).length());
	longest_v = std::max((verts[0][0] - verts[0][3]).length(), (verts[0][1] - verts[0][2]).length());
	log_widest = fastlog2(std::max(longest_u, longest_v));

	// Calculate bounds
	bbox.resize(verts.size());
	for (size_t time = 0; time < verts.size(); time++) {
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

const std::vector<BBox> &Bilinear::bounds() const
{
	return bbox;
}


#if 0
size_t Bilinear::subdiv_estimate(float width) const
{
	// Since we want to end up with the log-base-2 of
	// the division anyway, we just do the log first and
	// subtract.  Using a very approximate log2, but in
	// practice it works fine.
	const float rate = log_widest - fasterlog2(width * Config::dice_rate) + 1.0f;
	return std::max(rate, 0.0f);
}


int Bilinear::split(std::unique_ptr<DiceableSurface> objects[])
{
	auto patch1 = new Bilinear();
	auto patch2 = new Bilinear();

	// Split
	if (longest_u > longest_v) {
		// Split on U
		for (size_t i=0; i < verts.size(); i++) {
			patch1->add_time_sample(verts[i][0],
			                        (verts[i][0] + verts[i][1]) * 0.5,
			                        (verts[i][2] + verts[i][3]) * 0.5,
			                        verts[i][3]
			                       );
			patch2->add_time_sample((verts[i][0] + verts[i][1]) * 0.5,
			                        verts[i][1],
			                        verts[i][2],
			                        (verts[i][2] + verts[i][3]) * 0.5
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
		for (size_t i=0; i < verts.size(); i++) {
			patch1->add_time_sample(verts[i][0],
			                        verts[i][1],
			                        (verts[i][1] + verts[i][2]) * 0.5,
			                        (verts[i][3] + verts[i][0]) * 0.5
			                       );
			patch2->add_time_sample((verts[i][3] + verts[i][0]) * 0.5,
			                        (verts[i][1] + verts[i][2]) * 0.5,
			                        verts[i][2],
			                        verts[i][3]
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

	objects[0] = std::unique_ptr<DiceableSurface>(patch1);
	objects[1] = std::unique_ptr<DiceableSurface>(patch2);

	return 2;
}

std::unique_ptr<DiceableSurface> Bilinear::copy() const
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
	patch->log_widest = log_widest;

	return std::unique_ptr<DiceableSurface>(patch);
}


std::shared_ptr<MicroSurface> Bilinear::dice(size_t subdivisions) const
{
	// Get dicing rate
	const size_t rate = (1 << subdivisions) + 1;

	// Dice away!
	std::unique_ptr<Grid> grid = std::unique_ptr<Grid>(grid_dice(rate, rate));
	std::shared_ptr<MicroSurface> micro = std::make_shared<MicroSurface>(grid.get());

	return micro;
}


/*
 * Dice the patch into a micropoly grid.
 * ru and rv are the resolution of the grid in vertices
 * in the u and v directions.
 */
Grid *Bilinear::grid_dice(const int ru, const int rv) const
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
	size_t i, time;
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
				i = (ru*rv*time) + (ru*y+x);
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
#endif


void Bilinear::intersect_rays(const std::vector<Transform>& parent_xforms, Ray* ray_begin, Ray* ray_end, Intersection *intersections, Stack* data_stack)
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
	auto tmp = patch_stack.push_frame<std::array<Vec3, 4>>(tsc).first;
	for (int i = 0; i < tsc; ++i) {
		tmp[i] = verts[i];
	}
	uv_stack[0] = std::tuple<float, float, float, float>(u_min, u_max, v_min, v_max);

	// Iterate down to find an intersection
	while (stack_i >= 0) {
		auto cur_patches = patch_stack.top_frame<std::array<Vec3, 4>>().first;

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
			if (lerp_seq(ray.time, bboxes, bboxes+tsc).intersect_ray(ray, &hitt0, &hitt1, ray.max_t)) {
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
			auto next_patches = patch_stack.push_frame<std::array<Vec3, 4>>(tsc).first;

			const float ulen = longest_axis(cur_patches[0][0] - cur_patches[0][1]);
			const float vlen = longest_axis(cur_patches[0][0] - cur_patches[0][3]);

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
