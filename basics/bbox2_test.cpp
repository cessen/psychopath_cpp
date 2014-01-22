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
		BOOST_CHECK(bb.bounds[i][0] == std::numeric_limits<float>::infinity() &&
		            bb.bounds[i][1] == std::numeric_limits<float>::infinity() &&
		            bb.bounds[i][2] == -std::numeric_limits<float>::infinity() &&
		            bb.bounds[i][3] == -std::numeric_limits<float>::infinity());
	}
}

// Test for the second constructor
BOOST_AUTO_TEST_CASE(constructor_2)
{
	BBox2 bb(BBox(Vec3(1.0, -2.5, 0.5), Vec3(8.0, 7.25, 2.0)),
	         BBox(Vec3(2.0, -3.5, 1.5), Vec3(9.0, 8.25, 3.0)));

	BOOST_CHECK(bb.bounds[0][0] == 1.0);
	BOOST_CHECK(bb.bounds[0][1] == 2.0);
	BOOST_CHECK(bb.bounds[0][2] == 8.0);
	BOOST_CHECK(bb.bounds[0][3] == 9.0);

	BOOST_CHECK(bb.bounds[1][0] == -2.5);
	BOOST_CHECK(bb.bounds[1][1] == -3.5);
	BOOST_CHECK(bb.bounds[1][2] == 7.25);
	BOOST_CHECK(bb.bounds[1][3] == 8.25);

	BOOST_CHECK(bb.bounds[2][0] == 0.5);
	BOOST_CHECK(bb.bounds[2][1] == 1.5);
	BOOST_CHECK(bb.bounds[2][2] == 2.0);
	BOOST_CHECK(bb.bounds[2][3] == 3.0);
}


// Test for the add operator
BOOST_AUTO_TEST_CASE(add)
{
	BBox2 bb1(BBox(Vec3(1.0, -2.5, 0.5), Vec3(8.0, 7.25, 2.0)),
	          BBox(Vec3(2.0, -3.5, 1.5), Vec3(9.0, 8.25, 3.0)));
	BBox2 bb2(BBox(Vec3(-1.0, -1.5, -2.0), Vec3(8.0, 4.75, -1.0)),
	          BBox(Vec3(-1.5, -0.5, 1.0), Vec3(2.0, 7.75, 5.0)));

	BBox2 bb = bb1 + bb2;

	BOOST_CHECK(bb.bounds[0][0] == 0.0);
	BOOST_CHECK(bb.bounds[0][1] == 0.5);
	BOOST_CHECK(bb.bounds[0][2] == 16.0);
	BOOST_CHECK(bb.bounds[0][3] == 11.0);

	BOOST_CHECK(bb.bounds[1][0] == -4.0);
	BOOST_CHECK(bb.bounds[1][1] == -4.0);
	BOOST_CHECK(bb.bounds[1][2] == 12.0);
	BOOST_CHECK(bb.bounds[1][3] == 16.0);

	BOOST_CHECK(bb.bounds[2][0] == -1.5);
	BOOST_CHECK(bb.bounds[2][1] == 2.5);
	BOOST_CHECK(bb.bounds[2][2] == 1.0);
	BOOST_CHECK(bb.bounds[2][3] == 8.0);
}


// Test for the subtract operator
BOOST_AUTO_TEST_CASE(subtract)
{
	BBox2 bb1(BBox(Vec3(1.0, -2.5, 0.5), Vec3(8.0, 7.25, 2.0)),
	          BBox(Vec3(2.0, -3.5, 1.5), Vec3(9.0, 8.25, 3.0)));
	BBox2 bb2(BBox(Vec3(-1.0, -1.5, -2.0), Vec3(8.0, 4.75, -1.0)),
	          BBox(Vec3(-1.5, -0.5, 1.0), Vec3(2.0, 7.75, 5.0)));

	BBox2 bb = bb1 - bb2;

	BOOST_CHECK(bb.bounds[0][0] == 2.0);
	BOOST_CHECK(bb.bounds[0][1] == 3.5);
	BOOST_CHECK(bb.bounds[0][2] == 0.0);
	BOOST_CHECK(bb.bounds[0][3] == 7.0);

	BOOST_CHECK(bb.bounds[1][0] == -1.0);
	BOOST_CHECK(bb.bounds[1][1] == -3.0);
	BOOST_CHECK(bb.bounds[1][2] == 2.5);
	BOOST_CHECK(bb.bounds[1][3] == 0.5);

	BOOST_CHECK(bb.bounds[2][0] == 2.5);
	BOOST_CHECK(bb.bounds[2][1] == 0.5);
	BOOST_CHECK(bb.bounds[2][2] == 3.0);
	BOOST_CHECK(bb.bounds[2][3] == -2.0);
}


// Test for the multiply operator
BOOST_AUTO_TEST_CASE(multiply)
{
	BBox2 bb1(BBox(Vec3(1.0, -2.5, 0.5), Vec3(8.0, 7.25, 2.0)),
	          BBox(Vec3(2.0, -3.5, 1.5), Vec3(9.0, 8.25, 3.0)));

	BBox2 bb = bb1 * -2.0;

	BOOST_CHECK(bb.bounds[0][0] == -2.0);
	BOOST_CHECK(bb.bounds[0][1] == -4.0);
	BOOST_CHECK(bb.bounds[0][2] == -16.0);
	BOOST_CHECK(bb.bounds[0][3] == -18.0);

	BOOST_CHECK(bb.bounds[1][0] == 5.0);
	BOOST_CHECK(bb.bounds[1][1] == 7.0);
	BOOST_CHECK(bb.bounds[1][2] == -14.5);
	BOOST_CHECK(bb.bounds[1][3] == -16.5);

	BOOST_CHECK(bb.bounds[2][0] == -1.0);
	BOOST_CHECK(bb.bounds[2][1] == -3.0);
	BOOST_CHECK(bb.bounds[2][2] == -4.0);
	BOOST_CHECK(bb.bounds[2][3] == -6.0);
}


// Test for the divide operator
BOOST_AUTO_TEST_CASE(divide)
{
	BBox2 bb1(BBox(Vec3(1.0, -2.5, 0.5), Vec3(8.0, 7.25, 2.0)),
	          BBox(Vec3(2.0, -3.5, 1.5), Vec3(9.0, 8.25, 3.0)));

	BBox2 bb = bb1 / -2.0;

	BOOST_CHECK(bb.bounds[0][0] == -0.5);
	BOOST_CHECK(bb.bounds[0][1] == -1.0);
	BOOST_CHECK(bb.bounds[0][2] == -4.0);
	BOOST_CHECK(bb.bounds[0][3] == -4.5);

	BOOST_CHECK(bb.bounds[1][0] == 1.25);
	BOOST_CHECK(bb.bounds[1][1] == 1.75);
	BOOST_CHECK(bb.bounds[1][2] == -3.625);
	BOOST_CHECK(bb.bounds[1][3] == -4.125);

	BOOST_CHECK(bb.bounds[2][0] == -0.25);
	BOOST_CHECK(bb.bounds[2][1] == -0.75);
	BOOST_CHECK(bb.bounds[2][2] == -1.0);
	BOOST_CHECK(bb.bounds[2][3] == -1.5);
}


// Test for ::merge_with()
BOOST_AUTO_TEST_CASE(merge_with)
{
	BBox2 bb1(BBox(Vec3(1.0, -2.5, 0.5), Vec3(8.0, 7.25, 2.0)),
	          BBox(Vec3(2.0, -3.5, 1.5), Vec3(9.0, 8.25, 3.0)));
	BBox2 bb2(BBox(Vec3(-1.0, -1.5, -2.0), Vec3(8.0, 4.75, -1.0)),
	          BBox(Vec3(-1.5, -0.5, 1.0), Vec3(2.0, 7.75, 5.0)));

	bb1.merge_with(bb2);

	BOOST_CHECK(bb1.bounds[0][0] == -1.0);
	BOOST_CHECK(bb1.bounds[0][1] == -1.5);
	BOOST_CHECK(bb1.bounds[0][1] == -1.5);
	BOOST_CHECK(bb1.bounds[0][2] == 8.0);
	BOOST_CHECK(bb1.bounds[0][3] == 9.0);

	BOOST_CHECK(bb1.bounds[1][0] == -2.5);
	BOOST_CHECK(bb1.bounds[1][1] == -3.5);
	BOOST_CHECK(bb1.bounds[1][2] == 7.25);
	BOOST_CHECK(bb1.bounds[1][3] == 8.25);

	BOOST_CHECK(bb1.bounds[2][0] == -2.0);
	BOOST_CHECK(bb1.bounds[2][1] == 1.0);
	BOOST_CHECK(bb1.bounds[2][2] == 2.0);
	BOOST_CHECK(bb1.bounds[2][3] == 5.0);
}



// Tests for ::intersect_ray()
BOOST_AUTO_TEST_CASE(intersect_ray_1)
{
	// Simple intersection
	Ray r(Vec3(0.125, -8.0, 0.25), Vec3(0.0, 1.0, 0.0));
	r.finalize();
	BBox2 bb(BBox(Vec3(-1.0, -2.5, -0.5), Vec3(8.0, 7.25, 2.0)),
	         BBox(Vec3(-42.0, -3.5, -1.5), Vec3(9.0, 8.25, 1.0)));

	SIMD::float4 hit_ts;
	unsigned int hit_mask = bb.intersect_ray(r, &hit_ts);

	BOOST_CHECK(hit_mask == 3);
	BOOST_CHECK(hit_ts[0] == 5.5);
	BOOST_CHECK(hit_ts[1] == 4.5);
}


// Tests for ::intersect_ray()
BOOST_AUTO_TEST_CASE(intersect_ray_2)
{
	// Intersection with negative direction
	Ray r(Vec3(0.125, 12.0, 0.25), Vec3(0.0, -1.0, 0.0));
	r.finalize();
	BBox2 bb(BBox(Vec3(-1.0, -2.5, -0.5), Vec3(8.0, 7.25, 2.0)),
	         BBox(Vec3(-42.0, -3.5, -1.5), Vec3(9.0, 8.25, 1.0)));

	SIMD::float4 hit_ts;
	unsigned int hit_mask = bb.intersect_ray(r, &hit_ts);

	BOOST_CHECK(hit_mask == 3);
	BOOST_CHECK(hit_ts[0] == 4.75);
	BOOST_CHECK(hit_ts[1] == 3.75);
}


BOOST_AUTO_TEST_CASE(intersect_ray_3)
{
	// Simple intersection with unnormalized ray
	Ray r(Vec3(0.125, -8.0, 0.25), Vec3(0.0, 2.0, 0.0));
	r.update_accel();
	//r.finalize(); // Commented to avoid normalization of the ray
	BBox2 bb(BBox(Vec3(-1.0, -2.5, -0.5), Vec3(8.0, 7.25, 2.0)),
	         BBox(Vec3(-42.0, -3.5, -1.5), Vec3(9.0, 8.25, 1.0)));

	SIMD::float4 hit_ts;
	unsigned int hit_mask = bb.intersect_ray(r, &hit_ts);

	BOOST_CHECK(hit_mask == 3);
	BOOST_CHECK(hit_ts[0] == 2.75);
	BOOST_CHECK(hit_ts[1] == 2.25);
}

BOOST_AUTO_TEST_CASE(intersect_ray_4)
{
	// Simple miss
	Ray r(Vec3(-20.0, -8.0, 0.25), Vec3(0.0, 1.0, 0.0));
	r.finalize();
	BBox2 bb(BBox(Vec3(-1.0, -2.5, -0.5), Vec3(8.0, 7.25, 2.0)),
	         BBox(Vec3(-42.0, -3.5, -1.5), Vec3(9.0, 8.25, 1.0)));

	SIMD::float4 hit_ts;
	unsigned int hit_mask = bb.intersect_ray(r, &hit_ts);

	BOOST_CHECK(hit_mask == 2);
	BOOST_CHECK(hit_ts[1] == 4.5);
}

BOOST_AUTO_TEST_CASE(intersect_ray_5)
{
	// Intersection from ray that starts inside the bbox
	Ray r(Vec3(0.0, 0.0, 0.0), Vec3(0, 1.0, 0));
	r.finalize();
	BBox2 bb(BBox(Vec3(-1.0, -2.5, -0.5), Vec3(8.0, 7.25, 2.0)),
	         BBox(Vec3(-42.0, -3.5, -1.5), Vec3(9.0, 8.25, 1.0)));

	SIMD::float4 hit_ts;
	unsigned int hit_mask = bb.intersect_ray(r, &hit_ts);

	BOOST_CHECK(hit_mask == 3);
	BOOST_CHECK(hit_ts[0] == 0.0);
	BOOST_CHECK(hit_ts[1] == 0.0);
}

BOOST_AUTO_TEST_CASE(intersect_ray_6)
{
	// Intersection with collapsed BBox, should be true
	Ray r(Vec3(-4.0, 0.0, 0.0), Vec3(1.0, 0.0, 0.0));
	r.finalize();
	BBox2 bb(BBox(Vec3(1.0, -1.0, -1.0), Vec3(1.0, 1.0, 1.0)),
	         BBox(Vec3(2.0, -2.0, -2.0), Vec3(2.0, 2.0, 2.0)));

	SIMD::float4 hit_ts;
	unsigned int hit_mask = bb.intersect_ray(r, &hit_ts);

	BOOST_CHECK(hit_mask == 3);
	BOOST_CHECK(hit_ts[0] == 5.0);
	BOOST_CHECK(hit_ts[1] == 6.0);
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
//       - rays with different tmax values

BOOST_AUTO_TEST_SUITE_END()
