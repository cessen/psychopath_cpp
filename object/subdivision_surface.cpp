#include "subdivision_surface.hpp"
#include "patch_utils.hpp"


void SubdivisionSurface::intersect_rays(Ray* rays_begin, Ray* rays_end,
                                        Intersection *intersections,
                                        const Range<const Transform*> parent_xforms,
                                        Stack* data_stack,
                                        const SurfaceShader* surface_shader,
                                        const InstanceID& element_id
                                       ) const
{
	// TODO: proper BVH traversal, and bicubic patches
	for (const auto& patch: patches) {
		intersect_rays_with_patch<Bilinear>(patch, parent_xforms, rays_begin, rays_end, intersections, data_stack, surface_shader, element_id);
	}
}

void SubdivisionSurface::finalize()
{
	for (const auto& v: verts) {
		std::cout << v << " ";
	}

	// Calculate bounds
	BBox bb;
	for (const auto& v: verts) {
		bb.min = min(bb.min, v);
		bb.max = max(bb.max, v);
	}
	bbox.emplace_back(bb);

	// Extract bilinear patches from 4-vert faces
	// TODO: use OpenSubdiv to extract bicubic patches instead
	int vii = 0;
	for (const auto& fvc: face_vert_counts) {
		if (fvc == 4) {
			std::array<Vec3, 4> p;
			for (int i = 0; i < fvc; ++i) {
				p[i] = verts[face_vert_indices[vii+i]];
			}
			Bilinear bl(p[0], p[1], p[3], p[2]);
			bl.finalize();
			patches.emplace_back(bl);
		}

		vii += fvc;
	}

	// TODO: build bvh of patches
}