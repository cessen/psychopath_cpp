#ifndef SUBDIVISION_SURFACE_HPP
#define SUBDIVISION_SURFACE_HPP

#include <vector>

#include "object.hpp"
#include "intersection.hpp"
#include "ray.hpp"
#include "stack.hpp"
#include "vector.hpp"
#include "bbox.hpp"

class SubdivisionSurface final: public ComplexSurface
{
public:
	// Final data
	std::vector<std::array<Vec3, 16>> patches;
	std::vector<BBox> bbox;

	// Intermediate data
	std::vector<Vec3> verts;
	std::vector<int> face_vert_counts;
	std::vector<int> face_vert_indices;

	// Construction
	SubdivisionSurface() {}
	void set_verts(std::vector<Vec3>&& verts_) {
		verts = std::move(verts_);
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

	virtual void intersect_rays(const Ray* rays_begin, const Ray* rays_end,
	                            Intersection *intersections,
	                            const Range<const Transform*> parent_xforms,
	                            Stack* data_stack,
	                            const SurfaceShader* surface_shader,
	                            const InstanceID& element_id) const override;

};

#endif // SUBDIVISION_SURFACE_HPP