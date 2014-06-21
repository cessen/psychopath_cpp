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
	has_bounds = false;
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
	has_bounds = false;
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

	if (intersection && (ray.flags() & Ray::IS_OCCLUSION) == 0) {
		intersection->p = ray.o + (ray.d * t);
		intersection->t = t;
		intersection->n = intersection->p - cent;
		intersection->n.normalize();
		intersection->backfacing = dot(intersection->n, ray.d.normalized()) > 0.0f;
		intersection->in = ray.d;
		intersection->ow = ray.ow;
		intersection->dw = ray.dw;

		intersection->offset = Vec3(0.0f, 0.0f, 0.0f);

		intersection->col = Color(0.8, 0.8, 0.8);
	}

	return true;
}


std::vector<BBox> &Sphere::bounds()
{
	if (!has_bounds) {
		bbox.resize(center.size());

		for (size_t time = 0; time < center.size(); time++) {
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

