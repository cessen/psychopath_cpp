#include "subdivision_surface.hpp"

#include <iostream>
#include <cstring>
#include <vector>
#include <array>
#include <algorithm>

#include "patch_utils.hpp"

#include <opensubdiv/far/topologyDescriptor.h>
#include <opensubdiv/far/primvarRefiner.h>
#include <opensubdiv/far/patchTableFactory.h>
#include <opensubdiv/far/patchMap.h>


// Max depth of the BVH tree
static constexpr int DEPTH_LIMIT = 64;


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
	int stack_i = 0;
	SubdivisionSurface::Node* node_stack[DEPTH_LIMIT];
	Ray* ray_end_stack[DEPTH_LIMIT];

	node_stack[0] = bvh_root;
	ray_end_stack[0] = rays_end;

	while (stack_i >= 0) {
		assert(stack_i >= (DEPTH_LIMIT-1));

		// If node is not a leaf
		if (node_stack[stack_i]->leaf_data == nullptr) {
			// Test rays against current node
			ray_end_stack[stack_i] = mutable_partition(rays_begin, ray_end_stack[stack_i], [&](Ray& ray) {
				return lerp_seq(ray.time, node_stack[stack_i]->bounds).intersect_ray(ray);
			});

			// If any of the rays hit, proceed deeper with the ones that did
			if (rays_begin != ray_end_stack[stack_i]) {
				node_stack[stack_i+1] = node_stack[stack_i]->children[0];
				node_stack[stack_i] = node_stack[stack_i]->children[1];
				ray_end_stack[stack_i+1] = ray_end_stack[stack_i];
				++stack_i;
			}
			// Otherwise move up the stack
			else {
				--stack_i;
			}
		}
		// If node is a leaf
		else {
			intersect_rays_with_patch<Bicubic>(*(node_stack[stack_i]->leaf_data), parent_xforms, rays_begin, ray_end_stack[stack_i], intersections, data_stack, surface_shader, element_id);
			--stack_i;
		}
	}
}

void SubdivisionSurface::finalize()
{
	using namespace OpenSubdiv;
	constexpr int maxIsolation = 5; // Max depth of refinement of the subdiv mesh


	// Create a topology refiner, initialized from our mesh data
	Sdc::SchemeType type = Sdc::SCHEME_CATMARK;
	Sdc::Options options;
	options.SetVtxBoundaryInterpolation(Sdc::Options::VTX_BOUNDARY_EDGE_ONLY);

	Far::TopologyDescriptor desc;
	desc.numVertices = verts_per_motion_sample;
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
	const int nRefinerVertices = refiner->GetNumVerticesTotal();
	const int nLocalPoints = patchTable->GetNumLocalPoints();
	std::vector<SubdivVec3> patch_verts((nRefinerVertices + nLocalPoints) * motion_samples);
	for (int ms = 0; ms < motion_samples; ++ms) {
		std::memcpy(&patch_verts[(nRefinerVertices + nLocalPoints) * ms], &verts[verts_per_motion_sample*ms], verts_per_motion_sample*3*sizeof(float));

		SubdivVec3* src = reinterpret_cast<SubdivVec3*>(&patch_verts[(nRefinerVertices + nLocalPoints) * ms]);
		for (int level = 1; level <= maxIsolation; ++level) {
			SubdivVec3* dst = src + refiner->GetLevel(level-1).GetNumVertices();
			Far::PrimvarRefiner(*refiner).Interpolate(level, src, dst);
			src = dst;
		}
		patchTable->ComputeLocalPointValues(&patch_verts[(nRefinerVertices + nLocalPoints) * ms], &patch_verts[((nRefinerVertices + nLocalPoints) * ms) + nRefinerVertices]);
	}


	// Extract bicubic patches from the patch table
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

			// Get vertices for each time sample
			for (int ms = 0; ms < motion_samples; ++ms) {
				const int toffset = (nRefinerVertices + nLocalPoints) * ms;
				std::array<Vec3, 16> patch_verts {
					pvVec3[toffset + patch_vert_indices[0]],
					pvVec3[toffset + patch_vert_indices[1]],
					pvVec3[toffset + patch_vert_indices[2]],
					pvVec3[toffset + patch_vert_indices[3]],
					pvVec3[toffset + patch_vert_indices[4]],
					pvVec3[toffset + patch_vert_indices[5]],
					pvVec3[toffset + patch_vert_indices[6]],
					pvVec3[toffset + patch_vert_indices[7]],
					pvVec3[toffset + patch_vert_indices[8]],
					pvVec3[toffset + patch_vert_indices[9]],
					pvVec3[toffset + patch_vert_indices[10]],
					pvVec3[toffset + patch_vert_indices[11]],
					pvVec3[toffset + patch_vert_indices[12]],
					pvVec3[toffset + patch_vert_indices[13]],
					pvVec3[toffset + patch_vert_indices[14]],
					pvVec3[toffset + patch_vert_indices[15]]
				};

				// Modify patch verts based on boundary condition
				switch (boundary_bits) {
					case 0b0001:
						patch_verts[0] = patch_verts[4] * 2.0f - patch_verts[8];
						patch_verts[1] = patch_verts[5] * 2.0f - patch_verts[9];
						patch_verts[2] = patch_verts[6] * 2.0f - patch_verts[10];
						patch_verts[3] = patch_verts[7] * 2.0f - patch_verts[11];
						break;

					case 0b0010:
						patch_verts[3]  = patch_verts[2]  * 2.0f - patch_verts[1];
						patch_verts[7]  = patch_verts[6]  * 2.0f - patch_verts[5];
						patch_verts[11] = patch_verts[10] * 2.0f - patch_verts[9];
						patch_verts[15] = patch_verts[14] * 2.0f - patch_verts[13];
						break;

					case 0b0100:
						patch_verts[12] = patch_verts[8]  * 2.0f - patch_verts[4];
						patch_verts[13] = patch_verts[9]  * 2.0f - patch_verts[5];
						patch_verts[14] = patch_verts[10] * 2.0f - patch_verts[6];
						patch_verts[15] = patch_verts[11] * 2.0f - patch_verts[7];
						break;

					case 0b1000:
						patch_verts[0]  = patch_verts[1]  * 2.0f - patch_verts[2];
						patch_verts[4]  = patch_verts[5]  * 2.0f - patch_verts[6];
						patch_verts[8]  = patch_verts[9]  * 2.0f - patch_verts[10];
						patch_verts[12] = patch_verts[13] * 2.0f - patch_verts[14];
						break;

					case 0b0011:
						patch_verts[0]  = patch_verts[4]  * 2.0f - patch_verts[8];
						patch_verts[1]  = patch_verts[5]  * 2.0f - patch_verts[9];
						patch_verts[2]  = patch_verts[6]  * 2.0f - patch_verts[10];
						patch_verts[3]  = patch_verts[6]  * 3.0f - patch_verts[10] - patch_verts[4];
						patch_verts[7]  = patch_verts[6]  * 2.0f - patch_verts[4];
						patch_verts[11] = patch_verts[10] * 2.0f - patch_verts[9];
						patch_verts[15] = patch_verts[14] * 2.0f - patch_verts[13];
						break;

					case 0b0110:
						patch_verts[3]  = patch_verts[2]  * 2.0f - patch_verts[1];
						patch_verts[7]  = patch_verts[6]  * 2.0f - patch_verts[5];
						patch_verts[11] = patch_verts[10] * 2.0f - patch_verts[9];
						patch_verts[15] = patch_verts[10] * 3.0f - patch_verts[9] - patch_verts[6];
						patch_verts[14] = patch_verts[10] * 2.0f - patch_verts[6];
						patch_verts[13] = patch_verts[9]  * 2.0f - patch_verts[5];
						patch_verts[12] = patch_verts[8]  * 2.0f - patch_verts[4];
						break;

					case 0b1100:
						patch_verts[15] = patch_verts[11] * 2.0f - patch_verts[7];
						patch_verts[14] = patch_verts[10] * 2.0f - patch_verts[6];
						patch_verts[13] = patch_verts[9]  * 2.0f - patch_verts[5];
						patch_verts[12] = patch_verts[9]  * 3.0f - patch_verts[5] - patch_verts[10];
						patch_verts[8]  = patch_verts[9]  * 2.0f - patch_verts[10];
						patch_verts[4]  = patch_verts[5]  * 2.0f - patch_verts[6];
						patch_verts[0]  = patch_verts[1]  * 2.0f - patch_verts[2];

						break;

					case 0b1001:
						patch_verts[12] = patch_verts[13] * 2.0f - patch_verts[14];
						patch_verts[8]  = patch_verts[9]  * 2.0f - patch_verts[10];
						patch_verts[4]  = patch_verts[5]  * 2.0f - patch_verts[6];
						patch_verts[0]  = patch_verts[5]  * 3.0f - patch_verts[6] - patch_verts[9];
						patch_verts[1]  = patch_verts[5]  * 2.0f - patch_verts[9];
						patch_verts[2]  = patch_verts[6]  * 2.0f - patch_verts[10];
						patch_verts[3]  = patch_verts[7]  * 2.0f - patch_verts[11];
						break;

					default:
						break;
				}

				bspline_to_bezier_patch(&patch_verts);

				patches[pi].add_time_sample(patch_verts);
			}

			patches[pi].finalize();
		}
	}


	// Calculate bounds
	BBox bb;
	for (const auto& v: verts) {
		bb.min = min(bb.min, v);
		bb.max = max(bb.max, v);
	}
	bbox.emplace_back(bb);

	// Build bvh of patches
	build_bvh();
}




void SubdivisionSurface::build_bvh()
{
	// Make sure we have enough memory reserved for the nodes and bboxes.
	// This is super important to prevent iterator invalidation.
	bvh_nodes.reserve(patches.size() * 2);
	bvh_bboxes.reserve(patches.size() * 2 * motion_samples);

	// Make leaf nodes
	for (auto& patch: patches) {
		// Push bounds onto bounds array
		auto bbox_start_i = bvh_bboxes.size();
		for (auto& bbox: patch.bounds()) {
			bvh_bboxes.emplace_back(bbox);
		}

		// Build leaf node
		SubdivisionSurface::Node node;
		node.bounds = Range<BBox*>(&bvh_bboxes[bbox_start_i], &(*bvh_bboxes.end()));
		node.children[0] = nullptr;
		node.children[1] = nullptr;
		node.leaf_data = &patch;

		// Push leaf onto node array
		bvh_nodes.emplace_back(node);
	}

	// Recursively build bvh from leaf nodes
	depth = 1;
	max_depth = 1;
	bvh_root = build_bvh_recursive(&(*bvh_nodes.begin()), &(*bvh_nodes.end()));

	// Max sure max_depth isn't too large
	if (max_depth >= (DEPTH_LIMIT - 1)) {
		std::cout << "WARNING: BVH depth for subdivision surface (" << max_depth << ") is too high.  Render may be incorrect." << std::endl;
		std::cout << "There are a total of " << patches.size() << " patches after refinement." << std::endl;
	}
}

SubdivisionSurface::Node* SubdivisionSurface::build_bvh_recursive(SubdivisionSurface::Node* begin, SubdivisionSurface::Node* end)
{
	max_depth = std::max(depth, max_depth);

	if (begin+1 == end) {
		// LEAF
		return begin;
	} else {
		// Get bounds of the given leaf nodes' centroids.
		BBox center_bounds;
		for (auto node_itr = begin; node_itr < end; ++node_itr) {
			center_bounds = center_bounds | node_itr->bounds[0].center();
		}
		const Vec3 extent = center_bounds.max - center_bounds.min;

		// Find which axis to split the leaf nodes on
		int max_axis = 0;
		if (extent.y > extent.x)
			max_axis = 1;
		if (extent.z > extent.y)
			max_axis = 2;

		// Partition the leaf nodes
		const float pmid = (center_bounds.min[max_axis] + center_bounds.max[max_axis]) * 0.5f;
		auto mid_itr = std::partition(begin, end, [max_axis, pmid](const SubdivisionSurface::Node& bn) {
			return bn.bounds[0].center()[max_axis] < pmid;
		});

		if (mid_itr == begin)
			++mid_itr;

		// Create new node
		bvh_nodes.emplace_back(SubdivisionSurface::Node());
		auto& node = bvh_nodes.back();

		// Populate new node, further recursively building in the process
		++depth;
		node.children[0] = build_bvh_recursive(begin, mid_itr);
		node.children[1] = build_bvh_recursive(mid_itr, end);
		node.leaf_data = nullptr;
		--depth;

		// Calculate bounds of the node
		const int first_bbox_i = bvh_bboxes.size();
		for (int i = 0; i < node.children[0]->bounds.size(); ++i) {
			bvh_bboxes.emplace_back(node.children[0]->bounds[i] | node.children[1]->bounds[i]);
		}
		node.bounds = Range<BBox*>(&bvh_bboxes[first_bbox_i], &(*bvh_bboxes.end()));

		return &node;
	}
}