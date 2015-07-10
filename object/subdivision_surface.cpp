#include "subdivision_surface.hpp"

void SubdivisionSurface::intersect_rays(const Ray* rays_begin, const Ray* rays_end,
                                        Intersection *intersections,
                                        const Range<const Transform*> parent_xforms,
                                        Stack* data_stack,
                                        const SurfaceShader* surface_shader,
                                        const InstanceID& element_id
                                       ) const
{
	// TODO
}

void SubdivisionSurface::finalize()
{
	// TODO
}