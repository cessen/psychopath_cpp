#include "micro_surface.hpp"

void MicroSurface::init_from_grid(Grid *grid)
{
	// Allocate space for normals and uvs
	normals.resize(grid->res_u * grid->res_v * grid->time_count);
	uvs.resize(grid->res_u * grid->res_v * 2);

	// Store face ID
	face_id = grid->face_id;

	// Calculate uvs
	grid->calc_uvs(&(uvs[0]));

	// TODO: Calculate displacements

	// Calculate surface normals
	grid->calc_normals(&(normals[0]));

	// TODO: Build MicroSurface tree
}

bool MicroSurface::intersect_ray(const Ray &ray, Intersection *inter)
{
	return false;
}