#include "micro_surface.hpp"
#include "config.hpp"

#include <iostream>

#define IS_LEAF 1


bool MicroSurface::intersect_ray(const Ray &ray, Intersection *inter)
{
	uint32 todo[64];
	uint_i todo_offset = 0;
	uint_i node = 0;
	float32 tnear, tfar;
	bool hit = false;
	uint_i hit_node = 0;

	float32 t = ray.max_t;
	// Intersect with the MicroSurface
	while (true) {
		if (intersect_node(node, ray, &tnear, &tfar, &t)) {
			if (nodes[node].flags & IS_LEAF) {
				// Check if it counts as a hit
				const float32 hitt = (tnear + tfar) / 2;
				if (hitt < t) {
					hit = true;
					hit_node = node;
					t = hitt;

					// Early out for shadow rays
					if (ray.is_shadow_ray)
						break;
				}

				if (todo_offset == 0)
					break;

				node = todo[--todo_offset];
			} else {
				// Put far BVH node on todo stack, advance to near node
				todo[todo_offset++] = nodes[node].child_index + time_count;
				node = nodes[node].child_index;
			}
		} else {
			if (todo_offset == 0)
				break;

			node = todo[--todo_offset];
		}
	}

	// Calculate intersection data
	if (hit && !ray.is_shadow_ray) {
		//inter->hit = true;
		inter->p = ray.o + (ray.d * t);
		inter->in = ray.d;
		inter->t = t;

		// TODO: account for normal interpolation
		const uint_i index = nodes[hit_node].data_index;
		inter->n = normals[index*time_count];

		// TODO: account for uv interpolation
		inter->col = Color(uvs[index*2], uvs[index*2+1], 0.0f);

		inter->offset = inter->n * nodes[hit_node].bounds.diagonal();
	}

	return hit;
}


struct GridBVHBuildEntry {
	bool first; // Used to tell if it's the first or second child.
	uint_i i;

	uint_i u_start, u_end;
	uint_i v_start, v_end;
};

void MicroSurface::init_from_grid(Grid *grid)
{
	time_count = grid->time_count;
	res_u = grid->res_u;
	res_v = grid->res_v;

	Config::microelement_count += (res_u-1) * (res_v-1);
	Config::microsurface_count++;

	// Store face ID
	face_id = grid->face_id;

	// Calculate uvs
	uvs.resize(grid->res_u * grid->res_v * 2);
	grid->calc_uvs(&(uvs[0]));

	// TODO: Calculate displacements

	// Calculate surface normals
	normals.resize(grid->res_u * grid->res_v * grid->time_count);
	grid->calc_normals(&(normals[0]));

	////////////////////////////////
	// WIP: Build MicroSurface tree
	uint_i trav_count = 0;

	nodes.resize(grid->res_u * grid->res_v * grid->time_count * 2);
	uint_i next_node_i = time_count;

	GridBVHBuildEntry todo[64];
	// Prepare root todo item
	todo[0].i = 0;
	todo[0].u_start = 0;
	todo[0].u_end = grid->res_u - 2;
	todo[0].v_start = 0;
	todo[0].v_end = grid->res_v - 2;

	bool down = true; // Whether we're going up or down the stack
	uint_i i = 0;
	while (true) {
		trav_count++;
		// Going down
		if (down) {
			// Clear flags
			nodes[todo[i].i].flags = 0;

			// Calculate data indices
			nodes[todo[i].i].data_index = todo[i].v_start * res_u + todo[i].u_start;
			nodes[todo[i].i].data_du = 1 + todo[i].u_end - todo[i].v_start;
			nodes[todo[i].i].data_dv = 1 + todo[i].v_end - todo[i].v_start;

			// If leaf
			if (todo[i].u_start == todo[i].u_end && todo[i].v_start == todo[i].v_end) {
				// Set flags
				nodes[todo[i].i].flags |= IS_LEAF;

				// Build bboxes
				const uint_i u = todo[i].u_start;
				const uint_i v = todo[i].v_start;
				const uint_i vert1_i = (v * grid->res_u + u) * time_count;
				const uint_i vert2_i = (v * grid->res_u + u + 1) * time_count;
				const uint_i vert3_i = ((v+1) * grid->res_u + u) * time_count;
				const uint_i vert4_i = ((v+1) * grid->res_u + u + 1) * time_count;

				for (uint_i ti = 0; ti < time_count; ti++) {
					BBox bb;

					// Min
					bb.min =             grid->verts[vert1_i+ti];
					bb.min = min(bb.min, grid->verts[vert2_i+ti]);
					bb.min = min(bb.min, grid->verts[vert3_i+ti]);
					bb.min = min(bb.min, grid->verts[vert4_i+ti]);

					// Max
					bb.max =             grid->verts[vert1_i+ti];
					bb.max = max(bb.max, grid->verts[vert2_i+ti]);
					bb.max = max(bb.max, grid->verts[vert3_i+ti]);
					bb.max = max(bb.max, grid->verts[vert4_i+ti]);

					nodes[todo[i].i+ti].bounds = bb;
				}

				// If root, finished
				if (i == 0)
					break;

				// If first child, go to second child
				if (todo[i].first == true) {
					down = true;
					i++;
				}
				// If second child, go to parent
				else {
					down = false;
					i -= 3;
				}
			}
			// If not leaf
			else {
				// Determine split axis
				const bool split_u = (todo[i].u_end - todo[i].u_start) > (todo[i].v_end - todo[i].v_start);

				// Store first child's node index
				nodes[todo[i].i].child_index = next_node_i;

				// Create children's todos
				todo[i+2].i = next_node_i;
				todo[i+2].first = true;
				next_node_i += time_count;
				todo[i+3].i = next_node_i;
				todo[i+3].first = false;
				next_node_i += time_count;

				// Fill in uv extents for each child
				if (split_u == true) {
					// Keep v extents
					todo[i+2].v_start = todo[i+3].v_start = todo[i].v_start;
					todo[i+2].v_end = todo[i+3].v_end = todo[i].v_end;

					// Split on u
					todo[i+2].u_start = todo[i].u_start;
					todo[i+2].u_end = (todo[i].u_start + todo[i].u_end) / 2;

					todo[i+3].u_start = (todo[i].u_start + todo[i].u_end) / 2 + 1;
					todo[i+3].u_end = todo[i].u_end;
				} else {
					// Keep u extents
					todo[i+2].u_start = todo[i+3].u_start = todo[i].u_start;
					todo[i+2].u_end = todo[i+3].u_end = todo[i].u_end;

					// Split on v
					todo[i+2].v_start = todo[i].v_start;
					todo[i+2].v_end = (todo[i].v_start + todo[i].v_end) / 2;

					todo[i+3].v_start = (todo[i].v_start + todo[i].v_end) / 2 + 1;
					todo[i+3].v_end = todo[i].v_end;
				}

				// Go to first child
				down = true;
				i += 2;
			}
		}
		// Going up
		else {
			// Merge children's BBoxes into this node's BBoxes
			const uint_i self_i = todo[i].i;
			const uint_i child1_i = nodes[todo[i].i].child_index;
			const uint_i child2_i = nodes[todo[i].i].child_index + time_count;
			for (uint_i ti = 0; ti < time_count; ti++) {
				nodes[self_i+ti].bounds = nodes[child1_i+ti].bounds;
				nodes[self_i+ti].bounds.merge_with(nodes[child2_i+ti].bounds);
			}

			// If root node, we're done
			if (i == 0)
				break;

			// If first child, move to second child
			if (todo[i].first == true) {
				down = true;
				i++;
			}
			// If second child, move to parent
			else {
				down = false;
				i -= 3;
			}
		}
	}

	//std::cout << res_u*res_v << ": " << trav_count << ", " << trav_count / (float32)(res_u*res_v) << std::endl;

}

