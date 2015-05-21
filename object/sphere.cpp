#include "numtype.h"

#include <iostream>
#include <cstdlib>
#include "sphere.hpp"
#include "global.hpp"


/**
 * @brief Construct sphere with the given center and radius.
 *
 * @param center_ Center of the sphere.
 * @param radius_ Radius of the sphere.
 */
Sphere::Sphere(Vec3 center_, float radius_)
{
	center.resize(1);
	radius.resize(1);

	center[0] = center_;
	radius[0] = radius_;
}


/**
 * @brief Construct sphere with the given number of time samples (for motion blur).
 *
 * The time samples must then be filled in with centers and radii via
 * add_time_sample()
 *
 * @param res_time_ Number of time samples.
 */
Sphere::Sphere(uint8_t res_time_)
{
	center.resize(res_time_);
	radius.resize(res_time_);
}


/**
 * @brief Fills in a time sample with the given center and radius.
 *
 * @param samp Index of the time sample.
 * @param center_ Center of the sphere for this time sample.
 * @param radius_ Radius of the sphere for this time sample.
 */
void Sphere::add_time_sample(int samp, Vec3 center_, float radius_)
{
	center[samp] = center_;
	radius[samp] = radius_;
}


void Sphere::finalize()
{
	bbox.resize(center.size());

	for (size_t time = 0; time < center.size(); time++) {
		bbox[time].min.x = center[time].x - radius[time];
		bbox[time].max.x = center[time].x + radius[time];
		bbox[time].min.y = center[time].y - radius[time];
		bbox[time].max.y = center[time].y + radius[time];
		bbox[time].min.z = center[time].z - radius[time];
		bbox[time].max.z = center[time].z + radius[time];
	}
}


//////////////////////////////////////////////////////////////

bool Sphere::intersect_ray(const Ray &ray, Intersection *intersection)
{
	// Get the center and radius of the sphere at the ray's time
	const Vec3 cent = lerp_seq(ray.time, center); // Center of the sphere
	const float radi = lerp_seq(ray.time, radius); // Radius of the sphere

	// Calculate the relevant parts of the ray for the intersection
	Vec3 o = ray.o - cent; // Ray origin relative to sphere center
	Vec3 d = ray.d;


	// Code taken shamelessly from https://github.com/Tecla/Rayito
	// Ray-sphere intersection can result in either zero, one or two points
	// of intersection.  It turns into a quadratic equation, so we just find
	// the solution using the quadratic formula.  Note that there is a
	// slightly more stable form of it when computing it on a computer, and
	// we use that method to keep everything accurate.

	// Calculate quadratic coeffs
	float a = d.length2();
	float b = 2.0f * dot(d, o);
	float c = o.length2() - radi * radi;

	float t0, t1, discriminant;
	discriminant = b * b - 4.0f * a * c;
	if (discriminant < 0.0f) {
		// Discriminant less than zero?  No solution => no intersection.
		return false;
	}
	discriminant = std::sqrt(discriminant);

	// Compute a more stable form of our param t (t0 = q/a, t1 = c/q)
	// q = -0.5 * (b - sqrt(b * b - 4.0 * a * c)) if b < 0, or
	// q = -0.5 * (b + sqrt(b * b - 4.0 * a * c)) if b >= 0
	float q;
	if (b < 0.0f) {
		q = -0.5f * (b - discriminant);
	} else {
		q = -0.5f * (b + discriminant);
	}

	// Get our final parametric values
	t0 = q / a;
	if (q != 0.0f) {
		t1 = c / q;
	} else {
		t1 = ray.max_t;
	}

	// Swap them so they are ordered right
	if (t0 > t1) {
		float temp = t1;
		t1 = t0;
		t0 = temp;
	}

	// Check our intersection for validity against this ray's extents
	if (t0 >= ray.max_t || t1 < 0.0001f)
		return false;

	float t;
	if (t0 >= 0.0001f) {
		t = t0;
	} else if (t1 < ray.max_t) {
		t = t1;
	} else {
		return false;
	}

	if (intersection && !ray.is_occlusion()) {
		intersection->t = t;

		intersection->geo.p = ray.o + (ray.d * t);
		intersection->geo.n = intersection->geo.p - cent;
		intersection->geo.n.normalize();

		intersection->backfacing = dot(intersection->geo.n, ray.d.normalized()) > 0.0f;

		// Calculate the latitude and longitude of the hit point on the sphere
		const Vec3 unit_p = intersection->geo.n;
		const Vec3 p = unit_p * radi;
		const float lat_cos = unit_p.z;
		const float lat_sin = std::sqrt((unit_p.x * unit_p.x) + (unit_p.y * unit_p.y));
		const float long_cos = unit_p.x / lat_sin;
		const float long_sin = unit_p.y / lat_sin;

		float latitude = std::acos(lat_cos);
		float longitude = 0.0f;
		if (unit_p.x != 0.0f || unit_p.y != 0.0f) {
			longitude = std::acos(long_cos);
			if (unit_p.y < 0.0f)
				longitude = (2.0f * M_PI) - longitude;
		}

		// UV
		const float pi2 = M_PI * 2;
		intersection->geo.u = longitude / pi2;
		intersection->geo.v = latitude / M_PI;

		// Differential position
		intersection->geo.dpdu = Vec3(p.y * -1.0f, p.x, 0.0f) * pi2;
		intersection->geo.dpdv = Vec3(p.z * long_cos, p.z * long_sin, -radi * lat_sin) * M_PI;

		// Differential normal
		// Calculate second derivatives
		const Vec3 d2pduu = Vec3(p.x, p.y, 0.0f) * (-pi2 * pi2);
		const Vec3 d2pduv = Vec3(-long_sin, long_cos, 0.0f) * M_PI * p.z * pi2;
		const Vec3 d2pdvv = Vec3(p.x, p.y, p.z) * (-M_PI * M_PI);
		// Calculate surface normal derivatives
		const float E = dot(intersection->geo.dpdu, intersection->geo.dpdu);
		const float F = dot(intersection->geo.dpdu, intersection->geo.dpdv);
		const float G = dot(intersection->geo.dpdv, intersection->geo.dpdv);
		const float e = dot(intersection->geo.n, d2pduu);
		const float f = dot(intersection->geo.n, d2pduv);
		const float g = dot(intersection->geo.n, d2pdvv);
		const float invEGF2 = 1.0f / ((E*G) - (F*F));
		intersection->geo.dndu = (((f*F) - (e*G)) * invEGF2 * intersection->geo.dpdu) + (((e*F) - (f*E)) * invEGF2 * intersection->geo.dpdv);
		intersection->geo.dndv = (((g*F) - (f*G)) * invEGF2 * intersection->geo.dpdu) + (((f*F) - (g*E)) * invEGF2 * intersection->geo.dpdv);

		intersection->offset = intersection->geo.n * 0.000001f;
	}

	return true;
}


const std::vector<BBox> &Sphere::bounds() const
{
	return bbox;
}

