#include "numtype.h"

#include <iostream>
#include <stdlib.h>
#include "sphere.hpp"
#include "config.hpp"


/**
 * @brief Construct sphere with the given center and radius.
 *
 * @param center_ Center of the sphere.
 * @param radius_ Radius of the sphere.
 */
Sphere::Sphere(Vec3 center_, float32 radius_)
{
	has_bounds = false;
	center.init(1);
	radius.init(1);

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
Sphere::Sphere(uint8 res_time_)
{
	has_bounds = false;
	center.init(res_time_);
	radius.init(res_time_);
}


Sphere::~Sphere()
{
	return;
}


/**
 * @brief Fills in a time sample with the given center and radius.
 *
 * @param samp Index of the time sample.
 * @param center_ Center of the sphere for this time sample.
 * @param radius_ Radius of the sphere for this time sample.
 */
void Sphere::add_time_sample(int samp, Vec3 center_, float32 radius_)
{
	center[samp] = center_;
	radius[samp] = radius_;
}


//////////////////////////////////////////////////////////////

bool Sphere::intersect_ray(Ray &ray, Intersection *intersection)
{
	Config::primitive_ray_tests++;

	// Get the center and radius of the sphere at the ray's time
	int ia, ib;
	float32 alpha;
	Vec3 cent; // Center of the sphere
	float32 radi; // Radius of the sphere
	if (center.query_time(ray.time, &ia, &ib, &alpha)) {
		cent = lerp(alpha, center[ia], center[ib]);
		radi = lerp(alpha, radius[ia], radius[ib]);
	} else {
		cent = center[0];
		radi = radius[0];
	}

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
	float32 a = d.length2();
	float32 b = 2.0f * dot(d, o);
	float32 c = o.length2() - radi * radi;

	float32 t0, t1, discriminant;
	discriminant = b * b - 4.0f * a * c;
	if (discriminant < 0.0f) {
		// Discriminant less than zero?  No solution => no intersection.
		return false;
	}
	discriminant = std::sqrt(discriminant);

	// Compute a more stable form of our param t (t0 = q/a, t1 = c/q)
	// q = -0.5 * (b - sqrt(b * b - 4.0 * a * c)) if b < 0, or
	// q = -0.5 * (b + sqrt(b * b - 4.0 * a * c)) if b >= 0
	float32 q;
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
		float32 temp = t1;
		t1 = t0;
		t0 = temp;
	}

	// Check our intersection for validity against this ray's extents
	if (t0 >= ray.max_t || t1 < ray.min_t) {
		return false;
	}

	float32 t;
	if (t0 >= ray.min_t) {
		t = t0;
	} else if (t1 < ray.max_t) {
		t = t1;
	} else {
		return false;
	}

	// TODO: move this outside of primitive intersection routines
	ray.max_t = t;

	// Create our intersection data
	if (intersection) {
		intersection->p = ray.o + (ray.d * t);
		intersection->n = intersection->p - cent;
		intersection->n.normalize();
		intersection->in = ray.d;
		intersection->t = t;
		intersection->offset = Vec3(0.0f, 0.0f, 0.0f);

		intersection->col = Color((intersection->n.x+1.0)/2, (intersection->n.y+1.0)/2, (intersection->n.z+1.0)/2);
	}

	return true;
}


BBoxT &Sphere::bounds()
{
	if (!has_bounds) {
		bbox.init(center.state_count);

		for (int time = 0; time < center.state_count; time++) {
			bbox[time].min.x = center[time].x - radius[time];
			bbox[time].max.x = center[time].x + radius[time];
			bbox[time].min.y = center[time].y - radius[time];
			bbox[time].max.y = center[time].y + radius[time];
			bbox[time].min.z = center[time].z - radius[time];
			bbox[time].max.z = center[time].z + radius[time];
		}
		has_bounds = true;
	}

	return bbox;
}

