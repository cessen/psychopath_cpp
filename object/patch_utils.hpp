#ifndef PATCH_UTILS_HPP
#define PATCH_UTILS_HPP

#include <tuple>
#include <utility>

#include "vector.hpp"
#include "bbox.hpp"
#include "ray.hpp"
#include "intersection.hpp"
#include "stack.hpp"
#include "surface_shader.hpp"



#define SPLIT_STACK_SIZE 64

template <typename PATCH>
void intersect_rays_with_patch(const PATCH &patch, const Range<const Transform*> parent_xforms, Ray* ray_begin, Ray* ray_end, Intersection *intersections, Stack* data_stack, const SurfaceShader* surface_shader, const InstanceID& element_id) {
	const size_t tsc = patch.verts.size(); // Time sample count
	int stack_i = 0;
	std::pair<Ray*, Ray*> ray_stack[SPLIT_STACK_SIZE];
	BBox* bboxes = data_stack->push_frame<BBox>(tsc).first;
	Stack &patch_stack = *data_stack;
	std::tuple<float, float, float, float> uv_stack[SPLIT_STACK_SIZE]; // (min_u, max_u, min_v, max_v)

	// Initialize stacks
	// TODO: take into account ray time
	ray_stack[0] = std::make_pair(ray_begin, ray_end);
	auto tmp = patch_stack.push_frame<typename PATCH::store_type>(tsc).first;
	for (unsigned int i = 0; i < tsc; ++i) {
		tmp[i] = patch.verts[i];
	}
	uv_stack[0] = std::tuple<float, float, float, float>(0.0f, 1.0f, 0.0f, 1.0f);

	// Iterate down to find an intersection
	while (stack_i >= 0) {
		auto cur_patches = patch_stack.top_frame<typename PATCH::store_type>().first;

		// Calculate bounding boxes and max_dim
		bboxes[0] = PATCH::bound(cur_patches[0]);
		float max_dim = longest_axis(bboxes[0].max - bboxes[0].min);
		for (unsigned int i = 1; i < tsc; ++i) {
			bboxes[i] = PATCH::bound(cur_patches[i]);
			max_dim = std::max(max_dim, longest_axis(bboxes[i].max - bboxes[i].min));
		}

		// TEST RAYS AGAINST BBOX
		ray_stack[stack_i].first = mutable_partition(ray_stack[stack_i].first, ray_stack[stack_i].second, [&](Ray& ray) {
			if (ray.is_done()) {
				return true;
			}

			// Time interpolation values, which may be used twice in the case
			// that there is more than one time sample, so store them outside
			// of the if statement below.
			float t_time;
			size_t t_index = 0;
			float t_nalpha = 0.0f;

			// Ray test
			float hitt0, hitt1;
			bool hit;
			if (tsc == 1) {
				// If we only have one time sample, we can skip the bbox interpolation
				hit = bboxes[0].intersect_ray(ray, &hitt0, &hitt1, ray.max_t);
			} else {
				// If we have more than one time sample, we need to interpolate the bbox
				// before testing.
				t_time = ray.time * (tsc - 1);
				t_index = t_time;
				t_nalpha = t_time - t_index;
				hit = lerp(t_nalpha, bboxes[t_index], bboxes[t_index+1]).intersect_ray(ray, &hitt0, &hitt1, ray.max_t);
			}

			if (hit) {
				const float width = std::max(ray.min_width(hitt0, hitt1) * Config::dice_rate, Config::min_upoly_size);
				// LEAF, so we don't have to go deeper, regardless of whether
				// we hit it or not.
				if (max_dim <= width || stack_i == (SPLIT_STACK_SIZE-1)) {
					const float tt = (hitt0 + hitt1) * 0.5f;
					if (tt > 0.0f && tt < ray.max_t) {
						auto &inter = intersections[ray.id()];
						inter.hit = true;
						inter.id = element_id;
						if (ray.is_occlusion()) {
							ray.set_done_true();
						} else {
							// Get the time-interpolated patch, for calculating
							// surface derivatives and normals below
							typename PATCH::store_type ipatch;
							if (tsc == 1) {
								// If we only have one time sample, we can skip the interpolation
								ipatch = patch.verts[0];
							} else {
								// If we have more than one time sample, we need to interpolate the patch
								ipatch = PATCH::interpolate_patch(t_nalpha, patch.verts[t_index], patch.verts[t_index+1]);
							}


							// Fill in intersection and ray info
							ray.max_t = tt;

							const float u = (std::get<0>(uv_stack[stack_i]) + std::get<1>(uv_stack[stack_i])) * 0.5f;
							const float v = (std::get<2>(uv_stack[stack_i]) + std::get<3>(uv_stack[stack_i])) * 0.5f;
							const float offset = max_dim * 1.74f;

							inter.t = tt;

							inter.space = parent_xforms.size() > 0 ? lerp_seq(ray.time, parent_xforms) : Transform();

							inter.geo.p = ray.o + (ray.d * tt);
							inter.geo.u = u;
							inter.geo.v = v;

							// Surface normal and differential geometry
							std::tie(inter.geo.n, inter.geo.dpdu, inter.geo.dpdv, inter.geo.dndu, inter.geo.dndv) = PATCH::differential_geometry(ipatch, u, v);

							// Did te ray hit from the back-side of the surface?
							inter.backfacing = dot(inter.geo.n, ray.d.normalized()) > 0.0f;

							inter.offset = inter.geo.n * offset;

							// Do shading
							if (surface_shader != nullptr) {
								surface_shader->shade(&inter);
							} else {
								inter.surface_closure.init(EmitClosure(Color(1.0, 0.0, 1.0)));
							}
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
			auto next_patches = patch_stack.push_frame<typename PATCH::store_type>(tsc).first;

			const float ulen = PATCH::ulen(cur_patches[0]);
			const float vlen = PATCH::vlen(cur_patches[0]);

			// Split U
			if (ulen > vlen) {
				for (unsigned int i = 0; i < tsc; ++i) {
					PATCH::split_u(cur_patches[i], &(cur_patches[i]), &(next_patches[i]));
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
				for (unsigned int i = 0; i < tsc; ++i) {
					PATCH::split_v(cur_patches[i], &(cur_patches[i]), &(next_patches[i]));
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


// Modifies a bicubic patch in place to convert it from bspline to bezier.
static inline void bspline_to_bezier_curve(Vec3* v1, Vec3* v2, Vec3* v3, Vec3* v4) {
	const Vec3 tmp_v2 = *v2;
	*v1 = (*v1 * (1.0/3.0)) + (*v2 * (2.0/3.0));
	*v4 = (*v4 * (1.0/3.0)) + (*v3 * (2.0/3.0));
	*v2 = (*v2 * (2.0/3.0)) + (*v3 * (1.0/3.0));
	*v3 = (*v3 * (2.0/3.0)) + (tmp_v2 * (1.0/3.0));
	*v1 = (*v1 * 0.5f) + (*v2 * 0.5f);
	*v4 = (*v4 * 0.5f) + (*v3 * 0.5f);
}


// Modifies a bicubic patch in place to convert it from bspline to bezier.
static inline void bspline_to_bezier_patch(std::array<Vec3, 16>* patch) {
	for (int i = 0; i < 4; ++i) {
		int ii = i * 4;
		bspline_to_bezier_curve(&(*patch)[ii], &(*patch)[ii+1], &(*patch)[ii+2], &(*patch)[ii+3]);
	}

	for (int i = 0; i < 4; ++i) {
		bspline_to_bezier_curve(&(*patch)[i], &(*patch)[i+4], &(*patch)[i+8], &(*patch)[i+12]);
	}
}


#endif // PATCH_UTILS_HPP