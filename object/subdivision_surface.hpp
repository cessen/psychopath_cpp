#ifndef SUBDIVISION_SURFACE_HPP
#define SUBDIVISION_SURFACE_HPP

#include <vector>

#include "object.hpp"
#include "intersection.hpp"
#include "ray.hpp"
#include "stack.hpp"
#include "vector.hpp"
#include "bbox.hpp"
#include "bilinear.hpp"
#include "bicubic.hpp"

class SubdivisionSurface final: public ComplexSurface {
	struct Node {
		Range<BBox*> bounds;
		Node* children[2];
		Bicubic* leaf_data;
	};

	void build_bvh();
	Node* build_bvh_recursive(Node* begin, Node* end);

public:
	// Final data
	std::vector<Bicubic> patches;
	std::vector<BBox> bbox;
	std::vector<Node> bvh_nodes;
	std::vector<BBox> bvh_bboxes;
	Node* bvh_root;
	int max_depth;

	// Intermediate data
	int depth;
	int motion_samples = 0;
	int verts_per_motion_sample = 0;
	std::vector<Vec3> verts;
	std::vector<int> face_vert_counts;
	std::vector<int> face_vert_indices;

	// Construction
	SubdivisionSurface() {}
	void set_verts(std::vector<Vec3>&& verts_, int verts_per_motion_sample_) {
		verts = std::move(verts_);
		verts_per_motion_sample = verts_per_motion_sample_;
		motion_samples = verts.size() / verts_per_motion_sample;
	}
	void set_face_vert_counts(std::vector<int>&& vert_counts) {
		face_vert_counts = std::move(vert_counts);
	}
	void set_face_vert_indices(std::vector<int>&& vert_indices) {
		face_vert_indices = std::move(vert_indices);
	}
	void finalize();

	virtual const std::vector<BBox> &bounds() const override {
		return bbox;
	}

	virtual Color total_emitted_color() const override {
		return Color(0.0f);
	}

	virtual void intersect_rays(Ray* rays_begin, Ray* rays_end,
	                            Intersection *intersections,
	                            const Range<const Transform*> parent_xforms,
	                            Stack* data_stack,
	                            const SurfaceShader* surface_shader,
	                            const InstanceID& element_id) const override;

};

#endif // SUBDIVISION_SURFACE_HPP