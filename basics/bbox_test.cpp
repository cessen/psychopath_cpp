#include "test.hpp"

#include <cmath>
#include <limits>
#include <iostream>
#include "vector.hpp"
#include "ray.hpp"
#include "bbox.hpp"
#include "utils.hpp"


/*
 ************************************************************************
 * Testing suite for BBox.
 ************************************************************************
 */
BOOST_AUTO_TEST_SUITE(bounding_box_suite)


// Test for the first constructor
BOOST_AUTO_TEST_CASE(constructor_1)
{
	BBox bb;

	BOOST_CHECK(bb.min == Vec3(std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity()));
	BOOST_CHECK(bb.max == Vec3(-std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity()));
}

// Test for the second constructor
BOOST_AUTO_TEST_CASE(constructor_2)
{
	BBox bb(Vec3(1.0, -2.5, 0.5), Vec3(8.0, 7.25, 2.0));

	BOOST_CHECK(bb.min == Vec3(1.0, -2.5, 0.5));
	BOOST_CHECK(bb.max == Vec3(8.0, 7.25, 2.0));
}


// Test for the add operator
BOOST_AUTO_TEST_CASE(add)
{
	BBox bb1(Vec3(1.0, -2.5, 0.5), Vec3(8.0, 7.25, 2.0));
	BBox bb2(Vec3(-1.0, -1.5, -2.0), Vec3(8.0, 4.75, -1.0));

	BBox bb = bb1 + bb2;

	BOOST_CHECK(bb.min == Vec3(0.0, -4.0, -1.5));
	BOOST_CHECK(bb.max == Vec3(16.0, 12.0, 1.0));
}


// Test for the subtract operator
BOOST_AUTO_TEST_CASE(subtract)
{
	BBox bb1(Vec3(1.0, -2.5, 0.5), Vec3(8.0, 7.25, 2.0));
	BBox bb2(Vec3(-1.0, -1.5, -2.0), Vec3(8.0, 4.75, -1.0));

	BBox bb = bb1 - bb2;

	BOOST_CHECK(bb.min == Vec3(2.0, -1.0, 2.5));
	BOOST_CHECK(bb.max == Vec3(0.0, 2.5, 3.0));
}


// Test for the multiply operator
BOOST_AUTO_TEST_CASE(multiply)
{
	BBox bb1(Vec3(1.0, -2.5, 0.5), Vec3(8.0, 7.25, 2.0));

	BBox bb = bb1 * -2.0;

	BOOST_CHECK(bb.min == Vec3(-2.0, 5.0, -1.0));
	BOOST_CHECK(bb.max == Vec3(-16.0, -14.5, -4.0));
}


// Test for the divide operator
BOOST_AUTO_TEST_CASE(divide)
{
	BBox bb1(Vec3(1.0, -2.5, 0.5), Vec3(8.0, 7.25, 2.0));

	BBox bb = bb1 / -2.0;

	BOOST_CHECK(bb.min == Vec3(-0.5, 1.25, -0.25));
	BOOST_CHECK(bb.max == Vec3(-4.0, -3.625, -1.0));
}


// Test for ::merge_with()
BOOST_AUTO_TEST_CASE(merge_with)
{
	BBox bb1(Vec3(1.0, -2.5, 0.5), Vec3(8.0, 7.25, 2.0));
	BBox bb2(Vec3(-1.0, -1.5, -2.0), Vec3(8.0, 4.75, -1.0));

	bb1.merge_with(bb2);

	BOOST_CHECK(bb1.min == Vec3(-1.0, -2.5, -2.0));
	BOOST_CHECK(bb1.max == Vec3(8.0, 7.25, 2.0));
}


// Test for ::surface_area()
BOOST_AUTO_TEST_CASE(surface_area)
{
	BBox bb(Vec3(1.0, -2.5, 0.5), Vec3(8.0, 7.25, 2.0));

	BOOST_CHECK(bb.surface_area() == 186.75);
}


// Tests for ::intersect_ray()
BOOST_AUTO_TEST_CASE(intersect_ray_1)
{
	// Simple intersection
	Ray r(Vec3(0.125, -8.0, 0.25), Vec3(0.0, 1.0, 0.0));
	r.finalize();
	BBox bb(Vec3(-1.0, -2.5, -0.5), Vec3(8.0, 7.25, 2.0));
	float hitt0=0.0, hitt1=0.0;
	bool hit=false;

	hit = bb.intersect_ray(r, &hitt0, &hitt1);

	BOOST_CHECK(hit == true);
	BOOST_CHECK(hitt0 == 5.5);
	BOOST_CHECK(hitt1 >= 15.25);
	BOOST_CHECK(hitt1 <= 15.25001);
}


BOOST_AUTO_TEST_CASE(intersect_ray_2)
{
	// Simple intersection with unnormalized ray
	Ray r(Vec3(0.125, -8.0, 0.25), Vec3(0.0, 2.0, 0.0));
	r.update_accel();
	BBox bb(Vec3(-1.0, -2.5, -0.5), Vec3(8.0, 7.25, 2.0));
	float hitt0=0.0, hitt1=0.0;
	bool hit=false;

	hit = bb.intersect_ray(r, &hitt0, &hitt1);

	BOOST_CHECK(hit == true);
	BOOST_CHECK(hitt0 == 2.75);
	BOOST_CHECK(hitt1 >= 7.625);
	BOOST_CHECK(hitt1 <= (7.62501));
}

BOOST_AUTO_TEST_CASE(intersect_ray_3)
{
	// Simple miss
	Ray r(Vec3(20.0, -8.0, 0.25), Vec3(0.0, 1.0, 0.0));
	r.finalize();
	BBox bb(Vec3(-1.0, -2.5, -0.5), Vec3(8.0, 7.25, 2.0));

	BOOST_CHECK(bb.intersect_ray(r) == false);
}

BOOST_AUTO_TEST_CASE(intersect_ray_4)
{
	// Intersection from ray that starts inside the bbox
	Ray r(Vec3(0.0, 0.0, 0.0), Vec3(0, 1.0, 0));
	r.finalize();
	BBox bb(Vec3(-1.0, -2.5, -0.5), Vec3(8.0, 7.25, 2.0));
	float hitt0=0.0, hitt1=0.0;
	bool hit=false;

	hit = bb.intersect_ray(r, &hitt0, &hitt1);

	BOOST_CHECK(hit == true);
	BOOST_CHECK(hitt0 == 0.0);
	BOOST_CHECK(hitt1 >= 7.25);
	BOOST_CHECK(hitt1 <= 7.25001);
}

BOOST_AUTO_TEST_CASE(intersect_ray_5)
{
	// Intersection from ray that grazes the side of the bbox
	Ray r(Vec3(-1.0001, -8.0, 0.25), Vec3(0, 1.0, 0));
	r.finalize();
	BBox bb(Vec3(-1.0, -2.5, -0.5), Vec3(8.0, 7.25, 2.0));

	BOOST_CHECK(bb.intersect_ray(r) == false);
}

BOOST_AUTO_TEST_CASE(intersect_ray_6)
{
	// Intersection with collapsed BBox, should be true
	Ray r(Vec3(-4.0, 0.0, 0.0), Vec3(1.0, 0.0, 0.0));
	r.finalize();
	BBox bb(Vec3(1.0, -1.0, -1.0), Vec3(1.0, 1.0, 1.0));

	float hitt0=0.0, hitt1=0.0;
	bool hit=false;

	hit = bb.intersect_ray(r, &hitt0, &hitt1);

	BOOST_CHECK(hit == true);
	BOOST_CHECK(hitt0 == 5.0);
	BOOST_CHECK(hitt1 >= 5.0);
	BOOST_CHECK(hitt1 <= 5.00001);

}

BOOST_AUTO_TEST_CASE(intersect_ray_7)
{
	// Intersection with collapsed BBox with ray at an angle, should be true
	Ray r(Vec3(-4.0, 0.0, 0.0), Vec3(0.5, 0.5, 0.5));
	r.finalize();
	BBox bb(Vec3(1.0, -20.0, -20.0), Vec3(1.0, 20.0, 20.0));

	float hitt0=0.0, hitt1=0.0;
	bool hit=false;

	hit = bb.intersect_ray(r, &hitt0, &hitt1);

	BOOST_CHECK(hit == true);
	BOOST_CHECK(hitt0 == 10.0);
	BOOST_CHECK(hitt1 >= 10.0);
	BOOST_CHECK(hitt1 <= 10.00001);

}

// TODO: - diagonal rays
//       - rays with different tmin/tmax values


BOOST_AUTO_TEST_SUITE_END()


