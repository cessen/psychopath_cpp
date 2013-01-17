#ifndef DIFFERENTIAL_GEOMETRY_HPP
#define DIFFERENTIAL_GEOMETRY_HPP

class DifferentialGeometry {
	float32 u, v;
	
	Vec3 p;
	Vec3 dpdu, dpdv;
	
	Vec3 n;
	Vec3 dndu, dndv;
};

#endif // DIFFERENTIAL_GEOMETRY_HPP