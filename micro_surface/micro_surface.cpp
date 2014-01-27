#include "micro_surface.hpp"

#include <iostream>
#include <cmath>

#include "global.hpp"
#include "config.hpp"

#include "utils.hpp"

#define IS_LEAF    0b10000000
#define DEPTH_MASK 0b01111111


bool MicroSurface::intersect_ray(const Ray &ray, float ray_width, Intersection *inter)
{
	bool hit = false;
	size_t hit_node = 0;
	float hit_near = ray.max_t;
	//float hit_far = ray.max_t;
	float t = ray.max_t;
	if (inter)
		t = t < inter->t ? t : inter->t;

	// Calculate the max depth the ray should traverse into the tree
	const uint32_t rdepth = 2 * std::max(0.0f, fasterlog2(root_width) - fasterlog2(ray_width*Config::dice_rate));

	// Precalculated constants about the ray, for optimized BBox intersection
	const Vec3 d_inv = ray.get_d_inverse();
	const auto d_sign = ray.get_d_sign();

	assert(d_sign[0] < 2);
	assert(d_sign[1] < 2);
	assert(d_sign[2] < 2);

// Switch between simple traversal vs fast traversal (simple = 1, fast = 0)
#if 0
	// Intersect with the MicroSurface
	uint32_t todo[64];
	size_t todo_offset = 0;
	size_t node = 0;
	float tnear = ray.max_t;
	float tfar = ray.max_t;

	while (true) {
		if (intersect_node(node, ray, d_inv, d_sign, &tnear, &tfar, t)) {
			if (nodes[node].flags & IS_LEAF || (nodes[node].flags & DEPTH_MASK) >= rdepth) {
				// Hit
				hit = true;
				hit_node = node;
				t = tnear;
				hit_near = tnear;
				//hit_far = tfar;

				// Early out for shadow rays
				if (ray.is_shadow_ray)
					break;

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
#else
	// Intersect with the MicroSurface
	float tnear = 0.0f;
	float tfar = ray.max_t;

	// Working set
	uint64_t todo[64];
	float todo_t[64];
	int32_t stackptr = 0;

	// Test against the root node, and push it onto the stack
	todo[stackptr] = 0;
	if (intersect_node(todo[stackptr]*time_count, ray, d_inv, d_sign, &tnear, &tfar, t)) {
		todo_t[stackptr] = tnear;

		while (stackptr >= 0) {
			// Pop off the next node to work on.
			const int node_index = todo[stackptr];
			const float near = todo_t[stackptr];
			const MicroNode &node(nodes[node_index]);
			stackptr--;

			// If this node is further than the closest found intersection, continue
			if (near > t)
				continue;

			// If it's a leaf, store intersection information
			if (node.flags & IS_LEAF || (node.flags & DEPTH_MASK) >= rdepth) {
				hit = true;
				hit_node = node_index;
				t = near;
				hit_near = near;

				// Early out for shadow rays
				if (ray.is_shadow_ray)
					break;
			} else { // Not a leaf
				float hit_near1 = 0.0f; // Hit near 1
				float hit_near2 = 0.0f; // Hit near 2
				const bool hit1 = intersect_node(node.child_index, ray, d_inv, d_sign, &hit_near1, &tfar, t);
				const bool hit2 = intersect_node(node.child_index+time_count, ray, d_inv, d_sign, &hit_near2, &tfar, t);

				// Did we hit both nodes?
				if (hit1 && hit2) {
					if (hit_near1 < hit_near2) {
						// Left child is nearer
						// Push right first
						todo[++stackptr] = node.child_index + time_count;
						todo_t[stackptr] = hit_near2;

						todo[++stackptr] = node.child_index;
						todo_t[stackptr] = hit_near1;
					} else {
						// Right child is nearer
						// Push left first
						todo[++stackptr] = node.child_index;
						todo_t[stackptr] = hit_near1;

						todo[++stackptr] = node.child_index + time_count;
						todo_t[stackptr] = hit_near2;
					}
				} else if (hit1) {
					todo[++stackptr] = node.child_index;
					todo_t[stackptr] = hit_near1;
				} else if (hit2) {
					todo[++stackptr] = node.child_index + time_count;
					todo_t[stackptr] = hit_near2;
				}
			}
		}
	}
#endif

	// Calculate intersection data
	if (hit && !ray.is_shadow_ray) {
		if (t >= inter->t)
			return false;

		// Calculate time indices and alpha
		uint32_t t_i = 0;
		float t_alpha = 0.0f;
		calc_time_interp(time_count, ray.time, &t_i, &t_alpha);

		// Calculate data indices
		// TODO: something better than "727 % #".  We want to get a distributed
		// sampling over the uv space of the node.
		const uint d_iu = 727 % nodes[hit_node].data_du;
		const uint d_iv = 727 % nodes[hit_node].data_dv;
		const size_t d_index = nodes[hit_node].data_index; // Standard
		const size_t rd_index = d_index + (d_iv * res_u) + d_iu; // Random within range

		// Information about the intersection point
		inter->t = hit_near;
		inter->p = ray.o + (ray.d * hit_near);

		// Data about the ray that caused the intersection
		inter->in = ray.d;
		inter->ow = ray.ow;
		inter->dw = ray.dw;

		// Surface normal
		// TODO: differentials
		const Vec3 n1t1 = normals[rd_index*time_count+t_i];
		const Vec3 n2t1 = normals[(rd_index+1)*time_count+t_i];
		const Vec3 n3t1 = normals[(rd_index+res_u)*time_count+t_i];
		const Vec3 n4t1 = normals[(rd_index+res_u+1)*time_count+t_i];
		//const Vec3 nt1 = lerp2d<Vec3>(rng.next_float(), rng.next_float(), n1t1, n2t1, n3t1, n4t1);
		const Vec3 nt1 = lerp2d<Vec3>(0.5f, 0.5f, n1t1, n2t1, n3t1, n4t1);


		if (time_count > 1) {
			const Vec3 n1t2 = normals[rd_index*time_count+t_i+1];
			const Vec3 n2t2 = normals[(rd_index+1)*time_count+t_i+1];
			const Vec3 n3t2 = normals[(rd_index+res_u)*time_count+t_i+1];
			const Vec3 n4t2 = normals[(rd_index+res_u+1)*time_count+t_i+1];
			//const Vec3 nt2 = lerp2d<Vec3>(rng.next_float(), rng.next_float(), n1t2, n2t2, n3t2, n4t2);
			const Vec3 nt2 = lerp2d<Vec3>(0.5f, 0.5f, n1t2, n2t2, n3t2, n4t2);


			inter->n = lerp<Vec3>(t_alpha, nt1, nt2).normalized();
		} else {
			inter->n = nt1.normalized();
		}


		const float dl = std::max(ray.width(t) * Config::dice_rate * 1.5f, nodes[hit_node].bounds.diagonal());
		inter->offset = inter->n * dl * 1.0f; // Origin offset for next ray
		inter->backfacing = dot(inter->n, ray.d.normalized()) > 0.0f; // Whether the hit was on the back of the surface
		// UVs
		// TODO: differentials and correct coordinates for texturing
		inter->u = uvs[d_index*2];
		inter->v = uvs[d_index*2+1];

		// Color
		//inter->col = Color(inter->u, inter->v, 0.2f);
		inter->col = Color(0.8f, 0.8f, 0.8f);
	}

	return hit;
}


struct GridBVHBuildEntry {
	bool first; // Used to tell if it's the first or second child.
	size_t i;

	size_t u_start, u_end;
	size_t v_start, v_end;
};

void MicroSurface::init_from_grid(Grid *grid)
{
	time_count = grid->time_count;
	res_u = grid->res_u;
	res_v = grid->res_v;

	// Update statistics
	Global::Stats::microsurface_count++;
	const uint64_t element_count = (res_u-1) * (res_v-1);
	Global::Stats::microelement_count += element_count;
	if (element_count < Global::Stats::microelement_min_count)
		Global::Stats::microelement_min_count = element_count;
	if (element_count > Global::Stats::microelement_max_count)
		Global::Stats::microelement_max_count = element_count;

	// Store face ID
	face_id = grid->face_id;

	// Calculate uvs
	uvs.resize(grid->res_u * grid->res_v * 2);
	grid->calc_uvs(&(uvs[0]));

	// Calculate displacements
	// TODO: Use shaders for displacements
	/*
	normals.resize(grid->res_u * grid->res_v * grid->time_count);
	grid->calc_normals(&(normals[0]));
	for (size_t i = 0; i < res_u*res_v; i++) {
		for (size_t t = 0; t < time_count; t++) {
			grid->verts[i*time_count+t] += normals[i*time_count+t] * (cos(uvs[i*2]*3.14159*2)+cos(uvs[i*2+1]*3.14159*2)) * Config::displace_distance;
		}
	}
	*/

	// Calculate surface normals
	normals.resize(grid->res_u * grid->res_v * grid->time_count);
	grid->calc_normals(&(normals[0]));

	////////////////////////////////
	// WIP: Build MicroSurface tree
	size_t trav_count = 0;
	uint8_t depth = 0; // For tracking the depth of each node

	nodes.resize(grid->res_u * grid->res_v * grid->time_count * 2);
	size_t next_node_i = time_count;

	GridBVHBuildEntry todo[64];
	// Prepare root todo item
	todo[0].i = 0;
	todo[0].u_start = 0;
	todo[0].u_end = grid->res_u - 2;
	todo[0].v_start = 0;
	todo[0].v_end = grid->res_v - 2;

	bool down = true; // Whether we're going up or down the stack
	size_t i = 0;
	while (true) {
		trav_count++;
		// Going down
		if (down) {
			// Clear flags
			nodes[todo[i].i].flags = 0;
			// Store the depth of the node
			nodes[todo[i].i].flags = depth;

			// Calculate data indices
			nodes[todo[i].i].data_index = todo[i].v_start * res_u + todo[i].u_start;
			nodes[todo[i].i].data_du = 1 + todo[i].u_end - todo[i].u_start;
			nodes[todo[i].i].data_dv = 1 + todo[i].v_end - todo[i].v_start;

			// If leaf
			if (todo[i].u_start == todo[i].u_end && todo[i].v_start == todo[i].v_end) {
				// Set flags
				nodes[todo[i].i].flags |= IS_LEAF;

				// Build bboxes
				const size_t u = todo[i].u_start;
				const size_t v = todo[i].v_start;
				const size_t vert1_i = (v * grid->res_u + u) * time_count;
				const size_t vert2_i = (v * grid->res_u + u + 1) * time_count;
				const size_t vert3_i = ((v+1) * grid->res_u + u) * time_count;
				const size_t vert4_i = ((v+1) * grid->res_u + u + 1) * time_count;

				for (size_t ti = 0; ti < time_count; ti++) {
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
					depth--;
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
				depth++;
			}
		}
		// Going up
		else {
			// Merge children's BBoxes into this node's BBoxes
			const size_t self_i = todo[i].i;
			const size_t child1_i = nodes[todo[i].i].child_index;
			const size_t child2_i = nodes[todo[i].i].child_index + time_count;
			for (size_t ti = 0; ti < time_count; ti++) {
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
				depth--;
			}
		}
	}


	root_width = nodes[0].bounds.diagonal();
	//std::cout << res_u*res_v << ": " << trav_count << ", " << trav_count / (float)(res_u*res_v) << std::endl;

}

