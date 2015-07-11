#include "subdivision_surface.hpp"

#include <iostream>
#include <cstring>
#include <vector>
#include <array>

#include "patch_utils.hpp"

#include <opensubdiv/far/topologyDescriptor.h>
#include <opensubdiv/far/primvarRefiner.h>
#include <opensubdiv/far/patchTableFactory.h>
#include <opensubdiv/far/patchMap.h>


/*
 * A variation on Vec3 with the needed interface for OpenSubdiv.
 */
struct SubdivVec3 {
	SubdivVec3() { }

	void Clear(void * = 0) {
		p[0] = p[1] = p[2] = 0.0f;
	}

	void AddWithWeight(SubdivVec3 const & src, float weight) {
		p[0] += weight * src.p[0];
		p[1] += weight * src.p[1];
		p[2] += weight * src.p[2];
	}

	float p[3];
};


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
		intersect_rays_with_patch<Bicubic>(patch, parent_xforms, rays_begin, rays_end, intersections, data_stack, surface_shader, element_id);
	}
}

void SubdivisionSurface::finalize()
{
	using namespace OpenSubdiv;
	constexpr int maxIsolation = 0; // Max depth of refinement of the subdiv mesh


	// Create a topology refiner, initialized from our mesh data
	Sdc::SchemeType type = Sdc::SCHEME_CATMARK;
	Sdc::Options options;
	options.SetVtxBoundaryInterpolation(Sdc::Options::VTX_BOUNDARY_EDGE_ONLY);

	Far::TopologyDescriptor desc;
	desc.numVertices = verts.size();
	desc.numFaces = face_vert_counts.size();
	desc.numVertsPerFace = &(face_vert_counts[0]);
	desc.vertIndicesPerFace  = &(face_vert_indices[0]);

	std::unique_ptr<Far::TopologyRefiner> refiner = std::unique_ptr<Far::TopologyRefiner>(Far::TopologyRefinerFactory<Far::TopologyDescriptor>::Create(desc, Far::TopologyRefinerFactory<Far::TopologyDescriptor>::Options(type, options)));


	// Refine mesh topology and store it in a patch table
	refiner->RefineAdaptive(Far::TopologyRefiner::AdaptiveOptions(maxIsolation));
	Far::PatchTableFactory::Options patchOptions;
	patchOptions.endCapType = Far::PatchTableFactory::Options::ENDCAP_BSPLINE_BASIS;

	std::unique_ptr<Far::PatchTable> patchTable = std::unique_ptr<Far::PatchTable>(Far::PatchTableFactory::Create(*refiner, patchOptions));


	// Evaluate control points for the bicubic patches
	// TODO: motion blur
	const int nRefinerVertices = refiner->GetNumVerticesTotal();
	const int nLocalPoints = patchTable->GetNumLocalPoints();
	std::vector<SubdivVec3> patch_verts(nRefinerVertices + nLocalPoints);
	std::memcpy(&patch_verts[0], &verts[0], verts.size()*3*sizeof(float));

	SubdivVec3* src = reinterpret_cast<SubdivVec3*>(&patch_verts[0]);
	for (int level = 1; level <= maxIsolation; ++level) {
		SubdivVec3* dst = src + refiner->GetLevel(level-1).GetNumVertices();
		Far::PrimvarRefiner(*refiner).Interpolate(level, src, dst);
		src = dst;
	}
	patchTable->ComputeLocalPointValues(&patch_verts[0], &patch_verts[nRefinerVertices]);


	// Extract bicubic patches from the patch table
	// TODO: motion blur
	Vec3* pvVec3 = reinterpret_cast<Vec3*>(&patch_verts[0]);
	patches.clear();
	patches.resize(patchTable->GetNumPatchesTotal());
	// Loop through patch arrays
	for (int pa_i = 0; pa_i < patchTable->GetNumPatchArrays(); ++pa_i) {
		// Loop through patches in patch array
		for (int pi = 0; pi < patchTable->GetNumPatches(pa_i); ++pi) {
			const auto patch_params = patchTable->GetPatchParam(pa_i, pi);
			const auto boundary_bits = patch_params.GetBoundary();
			auto pvi = patchTable->GetPatchVertices(pa_i, pi);
			std::array<int, 16> patch_vert_indices;
			for (int i = 0; i < 16; ++i) {
				patch_vert_indices[i] = pvi[i];
			}

			std::cout << patch_vert_indices.size() << "\n";

			// Modify patch indices based on boundary condition
			switch (boundary_bits) {
				case 0b0001:
					patch_vert_indices[0] = patch_vert_indices[4];
					patch_vert_indices[1] = patch_vert_indices[5];
					patch_vert_indices[2] = patch_vert_indices[6];
					patch_vert_indices[3] = patch_vert_indices[7];
					break;

				case 0b0010:
					patch_vert_indices[3] = patch_vert_indices[2];
					patch_vert_indices[7] = patch_vert_indices[6];
					patch_vert_indices[11] = patch_vert_indices[10];
					patch_vert_indices[15] = patch_vert_indices[14];
					break;

				case 0b0100:
					patch_vert_indices[12] = patch_vert_indices[8];
					patch_vert_indices[13] = patch_vert_indices[9];
					patch_vert_indices[14] = patch_vert_indices[10];
					patch_vert_indices[15] = patch_vert_indices[11];
					break;

				case 0b1000:
					patch_vert_indices[0] = patch_vert_indices[1];
					patch_vert_indices[4] = patch_vert_indices[5];
					patch_vert_indices[8] = patch_vert_indices[9];
					patch_vert_indices[12] = patch_vert_indices[13];
					break;

				case 0b0011:
					patch_vert_indices[0] = patch_vert_indices[4];
					patch_vert_indices[1] = patch_vert_indices[5];
					patch_vert_indices[2] = patch_vert_indices[6];
					patch_vert_indices[3] = patch_vert_indices[6];
					patch_vert_indices[7] = patch_vert_indices[6];
					patch_vert_indices[11] = patch_vert_indices[10];
					patch_vert_indices[15] = patch_vert_indices[14];
					break;

				case 0b0110:
					patch_vert_indices[3] = patch_vert_indices[2];
					patch_vert_indices[7] = patch_vert_indices[6];
					patch_vert_indices[11] = patch_vert_indices[10];
					patch_vert_indices[15] = patch_vert_indices[10];
					patch_vert_indices[14] = patch_vert_indices[10];
					patch_vert_indices[13] = patch_vert_indices[9];
					patch_vert_indices[12] = patch_vert_indices[8];
					break;

				case 0b1100:
					patch_vert_indices[15] = patch_vert_indices[11];
					patch_vert_indices[14] = patch_vert_indices[10];
					patch_vert_indices[13] = patch_vert_indices[9];
					patch_vert_indices[12] = patch_vert_indices[9];
					patch_vert_indices[8] = patch_vert_indices[9];
					patch_vert_indices[4] = patch_vert_indices[5];
					patch_vert_indices[0] = patch_vert_indices[1];
					break;

				case 0b1001:
					patch_vert_indices[12] = patch_vert_indices[13];
					patch_vert_indices[8] = patch_vert_indices[9];
					patch_vert_indices[4] = patch_vert_indices[5];
					patch_vert_indices[0] = patch_vert_indices[5];
					patch_vert_indices[1] = patch_vert_indices[5];
					patch_vert_indices[2] = patch_vert_indices[6];
					patch_vert_indices[3] = patch_vert_indices[7];
					break;

				default:
					break;
			}

			patches[pi].add_time_sample(
			    pvVec3[patch_vert_indices[0]],
			    pvVec3[patch_vert_indices[1]],
			    pvVec3[patch_vert_indices[2]],
			    pvVec3[patch_vert_indices[3]],
			    pvVec3[patch_vert_indices[4]],
			    pvVec3[patch_vert_indices[5]],
			    pvVec3[patch_vert_indices[6]],
			    pvVec3[patch_vert_indices[7]],
			    pvVec3[patch_vert_indices[8]],
			    pvVec3[patch_vert_indices[9]],
			    pvVec3[patch_vert_indices[10]],
			    pvVec3[patch_vert_indices[11]],
			    pvVec3[patch_vert_indices[12]],
			    pvVec3[patch_vert_indices[13]],
			    pvVec3[patch_vert_indices[14]],
			    pvVec3[patch_vert_indices[15]]
			);

			patches[pi].finalize();
		}
	}




	//// Extract bilinear patches from 4-vert faces
	//// TODO: use OpenSubdiv to extract bicubic patches instead
	//int vii = 0;
	//for (const auto& fvc: face_vert_counts) {
	//	if (fvc == 4) {
	//		std::array<Vec3, 4> p;
	//		for (int i = 0; i < fvc; ++i) {
	//			p[i] = verts[face_vert_indices[vii+i]];
	//		}
	//		Bilinear bl(p[0], p[1], p[3], p[2]);
	//		bl.finalize();
	//		patches.emplace_back(bl);
	//	}
	//
	//	vii += fvc;
	//}


	// Calculate bounds
	BBox bb;
	for (const auto& v: verts) {
		bb.min = min(bb.min, v);
		bb.max = max(bb.max, v);
	}
	bbox.emplace_back(bb);

	// TODO: build bvh of patches
}