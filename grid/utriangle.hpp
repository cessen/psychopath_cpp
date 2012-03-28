#ifndef TRIANGLE_HPP
#define TRIANGLE_HPP

#include <stdlib.h>
#include "vector.h"

/* Not a true primitive.  Mainly a utility for tracing upoly grids.
 */
struct UTriangle {
    Vec3 verts[3];
    
    UTriangle(){}
    UTriangle(const Vec3 &v1, const Vec3 &v2, const Vec3 &v3);
    
    bool intersect_ray_(const Ray &ray,
			           float *t=NULL, float *u=NULL, float *v=NULL) const;
};

#endif
