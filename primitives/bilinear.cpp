#include "numtype.h"

#include <algorithm>
#include <iostream>
#include <stdlib.h>
#include "bilinear.hpp"
#include "grid.hpp"
#include "config.hpp"
#include "global.hpp"

Bilinear::Bilinear(uint16 res_time_)
{
	has_bounds = false;
	verts.init(res_time_);

	for (uint8 i=0; i < res_time_; i++) {
		verts[i] = new Vec3[4];
	}

	microsurface_key = 0;
	last_ray_width = 999999999999999.0f;
}

Bilinear::Bilinear(Vec3 v1, Vec3 v2, Vec3 v3, Vec3 v4)
{
	has_bounds = false;
	verts.init(1);
	verts[0] = new Vec3[4];

	verts[0][0] = v1;
	verts[0][1] = v2;
	verts[0][2] = v3;
	verts[0][3] = v4;

	microsurface_key = 0;
	last_ray_width = 999999999999999.0f;
}

Bilinear::~Bilinear()
{
	for (int i=0; i < verts.state_count; i++) {
		delete [] verts[i];
	}
}


void Bilinear::add_time_sample(int samp, Vec3 v1, Vec3 v2, Vec3 v3, Vec3 v4)
{
	verts[samp][0] = v1;
	verts[samp][1] = v2;
	verts[samp][2] = v3;
	verts[samp][3] = v4;
}


//////////////////////////////////////////////////////////////

uint_i Bilinear::micro_estimate(float32 width)
{
	if (width <= Config::min_upoly_size) {
		return 1;
	} else {
		uint_i u_rate = 0;
		uint_i v_rate = 0;
		uv_dice_rate(&u_rate, &v_rate, width);

		return u_rate * v_rate;
	}
}


bool Bilinear::intersect_ray(Ray &ray, Intersection *intersection)
{

	Global::Stats::primitive_ray_tests++;

	// Get bounding box intersection
	float32 tnear, tfar;
	if (!bounds().intersect_ray(ray, &tnear, &tfar))
		return false;

	// Calculate minimum ray footprint inside the bounding box
	const float32 width = ray.min_width(tnear, tfar);

	if (width == 0.0f) {
		/*std::cout << "____" << std::endl;
		std::cout << "Origin, Direction: " << ray.o << ", " << ray.d << std::endl;
		std::cout << "dX Origin, Direction: " << ray.odx << ", " << ray.ddx << std::endl;
		std::cout << "dY Origin, Direction: " << ray.ody << ", " << ray.ddy << std::endl;
		std::cout << "Is shadow ray: " << ray.is_shadow_ray << std::endl;
		*/
		//std::cout << "BOO\n";
	}

	// Figure out if we need to redice or not
	MicroSurface *micro_surface;
	bool redice = false;
	if (width < last_ray_width && width != 0.0f) {
		last_ray_width = width;
		redice = true;
	} else {
		// Try to get an existing grid
		micro_surface = MicroSurfaceCache::cache.open(microsurface_key);

		// Dice the grid if we don't have one already
		if (!micro_surface) {
			if (!(microsurface_key == 0))
				Global::Stats::cache_misses++;
			redice = true;
		}
	}

	if (redice) {
		// Redice
		micro_surface = micro_generate(width);
		microsurface_key = MicroSurfaceCache::cache.add_open(micro_surface);
	}

	// Test the ray against the grid
	const bool hit = micro_surface->intersect_ray(ray, intersection);
	MicroSurfaceCache::cache.close(microsurface_key);

	// Update ray if necessary
	if (hit && intersection)
		ray.max_t = intersection->t;

	return hit;
}


BBoxT &Bilinear::bounds()
{
	if (!has_bounds) {
		bbox.init(verts.state_count);

		for (int time = 0; time < verts.state_count; time++) {
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
		}
		has_bounds = true;
	}

	return bbox;
}


bool Bilinear::is_traceable()
{
	return true;
}


void Bilinear::split(std::vector<Primitive *> &primitives)
{
	primitives.resize(2);
	primitives[0] = new Bilinear(verts.state_count);
	primitives[1] = new Bilinear(verts.state_count);

	float32 lu;
	float32 lv;

	lu = (verts[0][0] - verts[0][1]).length() + (verts[0][3] - verts[0][2]).length();
	lv = (verts[0][0] - verts[0][3]).length() + (verts[0][1] - verts[0][2]).length();

	// TODO
	if (lu > lv) {
		// Split on U
		for (int i=0; i < verts.state_count; i++) {
			((Bilinear *)(primitives[0]))->add_time_sample(i,
			        verts[i][0],
			        (verts[i][0] + verts[i][1])*0.5,
			        (verts[i][2] + verts[i][3])*0.5,
			        verts[i][3]
			                                              );
			((Bilinear *)(primitives[1]))->add_time_sample(i,
			        (verts[i][0] + verts[i][1])*0.5,
			        verts[i][1],
			        verts[i][2],
			        (verts[i][2] + verts[i][3])*0.5
			                                              );
		}
	} else {
		// Split on V
		for (int i=0; i < verts.state_count; i++) {
			((Bilinear *)(primitives[0]))->add_time_sample(i,
			        verts[i][0],
			        verts[i][1],
			        (verts[i][1] + verts[i][2])*0.5,
			        (verts[i][3] + verts[i][0])*0.5
			                                              );
			((Bilinear *)(primitives[1]))->add_time_sample(i,
			        (verts[i][3] + verts[i][0])*0.5,
			        (verts[i][1] + verts[i][2])*0.5,
			        verts[i][2],
			        verts[i][3]
			                                              );
		}
	}
}


MicroSurface *Bilinear::micro_generate(float32 width)
{
	// Get dicing rate
	uint_i u_rate = 32;
	uint_i v_rate = 32;
	//uv_dice_rate(&u_rate, &v_rate, width);

	// TODO: this is temporary, while splitting is not yet implemented
	if (u_rate > Config::max_grid_size)
		u_rate = Config::max_grid_size;
	if (v_rate > Config::max_grid_size)
		v_rate = Config::max_grid_size;

	// Dice away!
	Grid *grid = dice(u_rate+1, v_rate+1);
	MicroSurface *micro = new MicroSurface(grid);

	delete grid;

	return micro;
}


/*
 * Dice the patch into a micropoly grid.
 * ru and rv are the resolution of the grid in vertices
 * in the u and v directions.
 */
Grid *Bilinear::dice(const int ru, const int rv)
{
	// Initialize grid and fill in the basics
	Grid *grid = new Grid(ru, rv, verts.state_count);

	// Fill in face and uvs
	// TODO: do this properly
	grid->face_id = 0;
	grid->u1 = 0.0;
	grid->v1 = 0.0;
	grid->u2 = 1.0;
	grid->v2 = 0.0;
	grid->u3 = 0.0;
	grid->v3 = 1.0;
	grid->u4 = 1.0;
	grid->v4 = 1.0;

	// Generate verts
	Vec3 du1;
	Vec3 du2;
	Vec3 dv;
	Vec3 p1, p2, p3;
	int x, y;
	int i, time;
	// Dice for each time sample
	for (time = 0; time < verts.state_count; time++) {
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
