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
 * Testing suite for BBox2.
 ************************************************************************
 */
BOOST_AUTO_TEST_SUITE(bounding_box_2_suite)


// Test for the first constructor
BOOST_AUTO_TEST_CASE(constructor_1)
{
	BBox2 bb;

	for (int i = 0; i < 3; ++i) {
		BOOST_CHECK(bb.bounds_f[i*4] == std::numeric_limits<float>::infinity() &&
		            bb.bounds_f[i*4+1] == std::numeric_limits<float>::infinity() &&
		            bb.bounds_f[i*4+2] == -std::numeric_limits<float>::infinity() &&
		            bb.bounds_f[i*4+3] == -std::numeric_limits<float>::infinity());
	}
}

// Test for the second constructor
BOOST_AUTO_TEST_CASE(constructor_2)
{
	BBox2 bb(BBox(Vec3(1.0, -2.5, 0.5), Vec3(8.0, 7.25, 2.0)),
	         BBox(Vec3(2.0, -3.5, 1.5), Vec3(9.0, 8.25, 3.0)));

	BOOST_CHECK(bb.bounds_f[0] == 1.0);
	BOOST_CHECK(bb.bounds_f[1] == 2.0);
	BOOST_CHECK(bb.bounds_f[2] == 8.0);
	BOOST_CHECK(bb.bounds_f[3] == 9.0);

	BOOST_CHECK(bb.bounds_f[4] == -2.5);
	BOOST_CHECK(bb.bounds_f[5] == -3.5);
	BOOST_CHECK(bb.bounds_f[6] == 7.25);
	BOOST_CHECK(bb.bounds_f[7] == 8.25);

	BOOST_CHECK(bb.bounds_f[8] == 0.5);
	BOOST_CHECK(bb.bounds_f[9] == 1.5);
	BOOST_CHECK(bb.bounds_f[10] == 2.0);
	BOOST_CHECK(bb.bounds_f[11] == 3.0);
}


// Test for the add operator
BOOST_AUTO_TEST_CASE(add)
{
	BBox2 bb1(BBox(Vec3(1.0, -2.5, 0.5), Vec3(8.0, 7.25, 2.0)),
	          BBox(Vec3(2.0, -3.5, 1.5), Vec3(9.0, 8.25, 3.0)));
	BBox2 bb2(BBox(Vec3(-1.0, -1.5, -2.0), Vec3(8.0, 4.75, -1.0)),
	          BBox(Vec3(-1.5, -0.5, 1.0), Vec3(2.0, 7.75, 5.0)));

	BBox2 bb = bb1 + bb2;

	BOOST_CHECK(bb.bounds_f[0] == 0.0);
	BOOST_CHECK(bb.bounds_f[1] == 0.5);
	BOOST_CHECK(bb.bounds_f[2] == 16.0);
	BOOST_CHECK(bb.bounds_f[3] == 11.0);

	BOOST_CHECK(bb.bounds_f[4] == -4.0);
	BOOST_CHECK(bb.bounds_f[5] == -4.0);
	BOOST_CHECK(bb.bounds_f[6] == 12.0);
	BOOST_CHECK(bb.bounds_f[7] == 16.0);

	BOOST_CHECK(bb.bounds_f[8] == -1.5);
	BOOST_CHECK(bb.bounds_f[9] == 2.5);
	BOOST_CHECK(bb.bounds_f[10] == 1.0);
	BOOST_CHECK(bb.bounds_f[11] == 8.0);
}


// Test for the subtract operator
BOOST_AUTO_TEST_CASE(subtract)
{
	BBox2 bb1(BBox(Vec3(1.0, -2.5, 0.5), Vec3(8.0, 7.25, 2.0)),
	          BBox(Vec3(2.0, -3.5, 1.5), Vec3(9.0, 8.25, 3.0)));
	BBox2 bb2(BBox(Vec3(-1.0, -1.5, -2.0), Vec3(8.0, 4.75, -1.0)),
	          BBox(Vec3(-1.5, -0.5, 1.0), Vec3(2.0, 7.75, 5.0)));

	BBox2 bb = bb1 - bb2;

	BOOST_CHECK(bb.bounds_f[0] == 2.0);
	BOOST_CHECK(bb.bounds_f[1] == 3.5);
	BOOST_CHECK(bb.bounds_f[2] == 0.0);
	BOOST_CHECK(bb.bounds_f[3] == 7.0);

	BOOST_CHECK(bb.bounds_f[4] == -1.0);
	BOOST_CHECK(bb.bounds_f[5] == -3.0);
	BOOST_CHECK(bb.bounds_f[6] == 2.5);
	BOOST_CHECK(bb.bounds_f[7] == 0.5);

	BOOST_CHECK(bb.bounds_f[8] == 2.5);
	BOOST_CHECK(bb.bounds_f[9] == 0.5);
	BOOST_CHECK(bb.bounds_f[10] == 3.0);
	BOOST_CHECK(bb.bounds_f[11] == -2.0);
}


// Test for the multiply operator
BOOST_AUTO_TEST_CASE(multiply)
{
	BBox2 bb1(BBox(Vec3(1.0, -2.5, 0.5), Vec3(8.0, 7.25, 2.0)),
	          BBox(Vec3(2.0, -3.5, 1.5), Vec3(9.0, 8.25, 3.0)));

	BBox2 bb = bb1 * -2.0;

	BOOST_CHECK(bb.bounds_f[0] == -2.0);
	BOOST_CHECK(bb.bounds_f[1] == -4.0);
	BOOST_CHECK(bb.bounds_f[2] == -16.0);
	BOOST_CHECK(bb.bounds_f[3] == -18.0);

	BOOST_CHECK(bb.bounds_f[4] == 5.0);
	BOOST_CHECK(bb.bounds_f[5] == 7.0);
	BOOST_CHECK(bb.bounds_f[6] == -14.5);
	BOOST_CHECK(bb.bounds_f[7] == -16.5);

	BOOST_CHECK(bb.bounds_f[8] == -1.0);
	BOOST_CHECK(bb.bounds_f[9] == -3.0);
	BOOST_CHECK(bb.bounds_f[10] == -4.0);
	BOOST_CHECK(bb.bounds_f[11] == -6.0);
}


// Test for the divide operator
BOOST_AUTO_TEST_CASE(divide)
{
	BBox2 bb1(BBox(Vec3(1.0, -2.5, 0.5), Vec3(8.0, 7.25, 2.0)),
	          BBox(Vec3(2.0, -3.5, 1.5), Vec3(9.0, 8.25, 3.0)));

	BBox2 bb = bb1 / -2.0;

	BOOST_CHECK(bb.bounds_f[0] == -0.5);
	BOOST_CHECK(bb.bounds_f[1] == -1.0);
	BOOST_CHECK(bb.bounds_f[2] == -4.0);
	BOOST_CHECK(bb.bounds_f[3] == -4.5);

	BOOST_CHECK(bb.bounds_f[4] == 1.25);
	BOOST_CHECK(bb.bounds_f[5] == 1.75);
	BOOST_CHECK(bb.bounds_f[6] == -3.625);
	BOOST_CHECK(bb.bounds_f[7] == -4.125);

	BOOST_CHECK(bb.bounds_f[8] == -0.25);
	BOOST_CHECK(bb.bounds_f[9] == -0.75);
	BOOST_CHECK(bb.bounds_f[10] == -1.0);
	BOOST_CHECK(bb.bounds_f[11] == -1.5);
}


// Test for ::merge_with()
BOOST_AUTO_TEST_CASE(merge_with)
{
	BBox2 bb1(BBox(Vec3(1.0, -2.5, 0.5), Vec3(8.0, 7.25, 2.0)),
	          BBox(Vec3(2.0, -3.5, 1.5), Vec3(9.0, 8.25, 3.0)));
	BBox2 bb2(BBox(Vec3(-1.0, -1.5, -2.0), Vec3(8.0, 4.75, -1.0)),
	          BBox(Vec3(-1.5, -0.5, 1.0), Vec3(2.0, 7.75, 5.0)));

	bb1.merge_with(bb2);

	BOOST_CHECK(bb1.bounds_f[0] == -1.0);
	BOOST_CHECK(bb1.bounds_f[1] == -1.5);
	BOOST_CHECK(bb1.bounds_f[2] == 8.0);
	BOOST_CHECK(bb1.bounds_f[3] == 9.0);

	BOOST_CHECK(bb1.bounds_f[4] == -2.5);
	BOOST_CHECK(bb1.bounds_f[5] == -3.5);
	BOOST_CHECK(bb1.bounds_f[6] == 7.25);
	BOOST_CHECK(bb1.bounds_f[7] == 8.25);

	BOOST_CHECK(bb1.bounds_f[8] == -2.0);
	BOOST_CHECK(bb1.bounds_f[9] == 1.0);
	BOOST_CHECK(bb1.bounds_f[10] == 2.0);
	BOOST_CHECK(bb1.bounds_f[11] == 5.0);
}



// Tests for ::intersect_ray()
BOOST_AUTO_TEST_CASE(intersect_ray_1)
{
	// Simple intersection
	Ray r(Vec3(0.125, -8.0, 0.25), Vec3(0.0, 1.0, 0.0));
	r.finalize();
	BBox2 bb(BBox(Vec3(-1.0, -2.5, -0.5), Vec3(8.0, 7.25, 2.0)),
	         BBox(Vec3(-42.0, -3.5, -1.5), Vec3(9.0, 8.25, 1.0)));

	float near_hits[2] = {std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity()};
	bool hit0 = false;
	bool hit1 = false;
	std::tie(hit0, hit1) = bb.intersect_ray(r, near_hits);

	BOOST_CHECK(hit0 == true);
	BOOST_CHECK(near_hits[0] == 5.5);
	BOOST_CHECK(hit1 == true);
	BOOST_CHECK(near_hits[1] == 4.5);
}

// Tests for ::intersect_ray()
BOOST_AUTO_TEST_CASE(intersect_ray_2)
{
	// Intersection with negative direction
	Ray r(Vec3(0.125, 12.0, 0.25), Vec3(0.0, -1.0, 0.0));
	r.finalize();
	BBox2 bb(BBox(Vec3(-1.0, -2.5, -0.5), Vec3(8.0, 7.25, 2.0)),
	         BBox(Vec3(-42.0, -3.5, -1.5), Vec3(9.0, 8.25, 1.0)));

	float near_hits[2] = {std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity()};
	bool hit0 = false;
	bool hit1 = false;
	std::tie(hit0, hit1) = bb.intersect_ray(r, near_hits);

	BOOST_CHECK(hit0 == true);
	BOOST_CHECK(near_hits[0] == 4.75);
	BOOST_CHECK(hit1 == true);
	BOOST_CHECK(near_hits[1] == 3.75);
}

BOOST_AUTO_TEST_CASE(intersect_ray_3)
{
	// Simple intersection with unnormalized ray
	Ray r(Vec3(0.125, -8.0, 0.25), Vec3(0.0, 2.0, 0.0));
	//r.finalize(); // Commented to avoid normalization of the ray
	BBox2 bb(BBox(Vec3(-1.0, -2.5, -0.5), Vec3(8.0, 7.25, 2.0)),
	         BBox(Vec3(-42.0, -3.5, -1.5), Vec3(9.0, 8.25, 1.0)));

	float near_hits[2] = {std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity()};
	bool hit0 = false;
	bool hit1 = false;
	std::tie(hit0, hit1) = bb.intersect_ray(r, near_hits);

	BOOST_CHECK(hit0 == true);
	BOOST_CHECK(near_hits[0] == 2.75);
	BOOST_CHECK(hit1 == true);
	BOOST_CHECK(near_hits[1] == 2.25);
}

BOOST_AUTO_TEST_CASE(intersect_ray_4)
{
	// Simple miss
	Ray r(Vec3(-20.0, -8.0, 0.25), Vec3(0.0, 1.0, 0.0));
	r.finalize();
	BBox2 bb(BBox(Vec3(-1.0, -2.5, -0.5), Vec3(8.0, 7.25, 2.0)),
	         BBox(Vec3(-42.0, -3.5, -1.5), Vec3(9.0, 8.25, 1.0)));

	float near_hits[2] = {std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity()};
	bool hit0 = false;
	bool hit1 = false;
	std::tie(hit0, hit1) = bb.intersect_ray(r, near_hits);

	BOOST_CHECK(hit0 == false);
	BOOST_CHECK(hit1 == true);
	BOOST_CHECK(near_hits[1] == 4.5);
}

BOOST_AUTO_TEST_CASE(intersect_ray_5)
{
	// Intersection from ray that starts inside the bbox
	Ray r(Vec3(0.0, 0.0, 0.0), Vec3(0, 1.0, 0));
	r.finalize();
	BBox2 bb(BBox(Vec3(-1.0, -2.5, -0.5), Vec3(8.0, 7.25, 2.0)),
	         BBox(Vec3(-42.0, -3.5, -1.5), Vec3(9.0, 8.25, 1.0)));

	float near_hits[2] = {std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity()};
	bool hit0 = false;
	bool hit1 = false;
	std::tie(hit0, hit1) = bb.intersect_ray(r, near_hits);

	BOOST_CHECK(hit0 == true);
	BOOST_CHECK(near_hits[0] == 0.0);
	BOOST_CHECK(hit1 == true);
	BOOST_CHECK(near_hits[1] == 0.0);
}

BOOST_AUTO_TEST_CASE(intersect_ray_6)
{
	// Intersection with collapsed BBox, should be true
	Ray r(Vec3(-4.0, 0.0, 0.0), Vec3(1.0, 0.0, 0.0));
	r.finalize();
	BBox2 bb(BBox(Vec3(1.0, -1.0, -1.0), Vec3(1.0, 1.0, 1.0)),
	         BBox(Vec3(2.0, -2.0, -2.0), Vec3(2.0, 2.0, 2.0)));

	float near_hits[2] = {std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity()};
	bool hit0 = false;
	bool hit1 = false;
	std::tie(hit0, hit1) = bb.intersect_ray(r, near_hits);

	BOOST_CHECK(hit0 == true);
	BOOST_CHECK(near_hits[0] == 5.0);
	BOOST_CHECK(hit1 == true);
	BOOST_CHECK(near_hits[1] == 6.0);
}
#if 0
BOOST_AUTO_TEST_CASE(intersect_ray_7)
{
	// Intersection from ray that grazes the side of the bbox
	Ray r(Vec3(-1.0, -8.0, 0.25), Vec3(0, 1.0, 0));
	r.finalize();
	BBox bb(Vec3(-1.0, -2.5, -0.5), Vec3(8.0, 7.25, 2.0));

	BOOST_CHECK(bb.intersect_ray(r) == false);
}



#endif

// TODO: - diagonal rays
//       - rays with different tmin/tmax values

BOOST_AUTO_TEST_SUITE_END()
