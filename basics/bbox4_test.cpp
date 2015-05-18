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
 * Testing suite for BBox4.
 ************************************************************************
 */


TEST_CASE("bbox4")
{
	// Test for the first constructor
	SECTION("constructor_1") {
		BBox4 bb;

		for (int i = 0; i < 6; i+=2) {
			bool t1 = bb.bounds[i][0] == std::numeric_limits<float>::infinity() &&
			          bb.bounds[i][1] == std::numeric_limits<float>::infinity() &&
			          bb.bounds[i][2] == std::numeric_limits<float>::infinity() &&
			          bb.bounds[i][3] == std::numeric_limits<float>::infinity();
			REQUIRE(t1);
			bool t2 = bb.bounds[i+1][0] == -std::numeric_limits<float>::infinity() &&
			          bb.bounds[i+1][1] == -std::numeric_limits<float>::infinity() &&
			          bb.bounds[i+1][2] == -std::numeric_limits<float>::infinity() &&
			          bb.bounds[i+1][3] == -std::numeric_limits<float>::infinity();
			REQUIRE(t2);
		}
	}

	// Test for the second constructor
	SECTION("constructor_2") {
		BBox4 bb(BBox(Vec3(1.0, 5.0, 9.0),  Vec3(13.0, 17.0, 21.0)),
		         BBox(Vec3(2.0, 6.0, 10.0), Vec3(14.0, 18.0, 22.0)),
		         BBox(Vec3(3.0, 7.0, 11.0), Vec3(15.0, 19.0, 23.0)),
		         BBox(Vec3(4.0, 8.0, 12.0), Vec3(16.0, 20.0, 24.0)));

		REQUIRE(bb.bounds[0][0] == 1.0);
		REQUIRE(bb.bounds[0][1] == 2.0);
		REQUIRE(bb.bounds[0][2] == 3.0);
		REQUIRE(bb.bounds[0][3] == 4.0);

		REQUIRE(bb.bounds[1][0] == 13.0);
		REQUIRE(bb.bounds[1][1] == 14.0);
		REQUIRE(bb.bounds[1][2] == 15.0);
		REQUIRE(bb.bounds[1][3] == 16.0);

		REQUIRE(bb.bounds[2][0] == 5.0);
		REQUIRE(bb.bounds[2][1] == 6.0);
		REQUIRE(bb.bounds[2][2] == 7.0);
		REQUIRE(bb.bounds[2][3] == 8.0);

		REQUIRE(bb.bounds[3][0] == 17.0);
		REQUIRE(bb.bounds[3][1] == 18.0);
		REQUIRE(bb.bounds[3][2] == 19.0);
		REQUIRE(bb.bounds[3][3] == 20.0);

		REQUIRE(bb.bounds[4][0] == 9.0);
		REQUIRE(bb.bounds[4][1] == 10.0);
		REQUIRE(bb.bounds[4][2] == 11.0);
		REQUIRE(bb.bounds[4][3] == 12.0);

		REQUIRE(bb.bounds[5][0] == 21.0);
		REQUIRE(bb.bounds[5][1] == 22.0);
		REQUIRE(bb.bounds[5][2] == 23.0);
		REQUIRE(bb.bounds[5][3] == 24.0);
	}

#if 0
	// Test for the add operator
	SECTION("add") {
		BBox2 bb1(BBox(Vec3(1.0, -2.5, 0.5), Vec3(8.0, 7.25, 2.0)),
		          BBox(Vec3(2.0, -3.5, 1.5), Vec3(9.0, 8.25, 3.0)));
		BBox2 bb2(BBox(Vec3(-1.0, -1.5, -2.0), Vec3(8.0, 4.75, -1.0)),
		          BBox(Vec3(-1.5, -0.5, 1.0), Vec3(2.0, 7.75, 5.0)));

		BBox2 bb = bb1 + bb2;

		REQUIRE(bb.bounds[0][0] == 0.0);
		REQUIRE(bb.bounds[0][1] == 0.5);
		REQUIRE(bb.bounds[0][2] == 16.0);
		REQUIRE(bb.bounds[0][3] == 11.0);

		REQUIRE(bb.bounds[1][0] == -4.0);
		REQUIRE(bb.bounds[1][1] == -4.0);
		REQUIRE(bb.bounds[1][2] == 12.0);
		REQUIRE(bb.bounds[1][3] == 16.0);

		REQUIRE(bb.bounds[2][0] == -1.5);
		REQUIRE(bb.bounds[2][1] == 2.5);
		REQUIRE(bb.bounds[2][2] == 1.0);
		REQUIRE(bb.bounds[2][3] == 8.0);
	}


	// Test for the subtract operator
	SECTION("subtract") {
		BBox2 bb1(BBox(Vec3(1.0, -2.5, 0.5), Vec3(8.0, 7.25, 2.0)),
		          BBox(Vec3(2.0, -3.5, 1.5), Vec3(9.0, 8.25, 3.0)));
		BBox2 bb2(BBox(Vec3(-1.0, -1.5, -2.0), Vec3(8.0, 4.75, -1.0)),
		          BBox(Vec3(-1.5, -0.5, 1.0), Vec3(2.0, 7.75, 5.0)));

		BBox2 bb = bb1 - bb2;

		REQUIRE(bb.bounds[0][0] == 2.0);
		REQUIRE(bb.bounds[0][1] == 3.5);
		REQUIRE(bb.bounds[0][2] == 0.0);
		REQUIRE(bb.bounds[0][3] == 7.0);

		REQUIRE(bb.bounds[1][0] == -1.0);
		REQUIRE(bb.bounds[1][1] == -3.0);
		REQUIRE(bb.bounds[1][2] == 2.5);
		REQUIRE(bb.bounds[1][3] == 0.5);

		REQUIRE(bb.bounds[2][0] == 2.5);
		REQUIRE(bb.bounds[2][1] == 0.5);
		REQUIRE(bb.bounds[2][2] == 3.0);
		REQUIRE(bb.bounds[2][3] == -2.0);
	}


	// Test for the multiply operator
	SECTION("multiply") {
		BBox2 bb1(BBox(Vec3(1.0, -2.5, 0.5), Vec3(8.0, 7.25, 2.0)),
		          BBox(Vec3(2.0, -3.5, 1.5), Vec3(9.0, 8.25, 3.0)));

		BBox2 bb = bb1 * -2.0;

		REQUIRE(bb.bounds[0][0] == -2.0);
		REQUIRE(bb.bounds[0][1] == -4.0);
		REQUIRE(bb.bounds[0][2] == -16.0);
		REQUIRE(bb.bounds[0][3] == -18.0);

		REQUIRE(bb.bounds[1][0] == 5.0);
		REQUIRE(bb.bounds[1][1] == 7.0);
		REQUIRE(bb.bounds[1][2] == -14.5);
		REQUIRE(bb.bounds[1][3] == -16.5);

		REQUIRE(bb.bounds[2][0] == -1.0);
		REQUIRE(bb.bounds[2][1] == -3.0);
		REQUIRE(bb.bounds[2][2] == -4.0);
		REQUIRE(bb.bounds[2][3] == -6.0);
	}


	// Test for the divide operator
	SECTION("divide") {
		BBox2 bb1(BBox(Vec3(1.0, -2.5, 0.5), Vec3(8.0, 7.25, 2.0)),
		          BBox(Vec3(2.0, -3.5, 1.5), Vec3(9.0, 8.25, 3.0)));

		BBox2 bb = bb1 / -2.0;

		REQUIRE(bb.bounds[0][0] == -0.5);
		REQUIRE(bb.bounds[0][1] == -1.0);
		REQUIRE(bb.bounds[0][2] == -4.0);
		REQUIRE(bb.bounds[0][3] == -4.5);

		REQUIRE(bb.bounds[1][0] == 1.25);
		REQUIRE(bb.bounds[1][1] == 1.75);
		REQUIRE(bb.bounds[1][2] == -3.625);
		REQUIRE(bb.bounds[1][3] == -4.125);

		REQUIRE(bb.bounds[2][0] == -0.25);
		REQUIRE(bb.bounds[2][1] == -0.75);
		REQUIRE(bb.bounds[2][2] == -1.0);
		REQUIRE(bb.bounds[2][3] == -1.5);
	}


	// Test for ::merge_with()
	SECTION("merge_with") {
		BBox2 bb1(BBox(Vec3(1.0, -2.5, 0.5), Vec3(8.0, 7.25, 2.0)),
		          BBox(Vec3(2.0, -3.5, 1.5), Vec3(9.0, 8.25, 3.0)));
		BBox2 bb2(BBox(Vec3(-1.0, -1.5, -2.0), Vec3(8.0, 4.75, -1.0)),
		          BBox(Vec3(-1.5, -0.5, 1.0), Vec3(2.0, 7.75, 5.0)));

		bb1.merge_with(bb2);

		REQUIRE(bb1.bounds[0][0] == -1.0);
		REQUIRE(bb1.bounds[0][1] == -1.5);
		REQUIRE(bb1.bounds[0][1] == -1.5);
		REQUIRE(bb1.bounds[0][2] == 8.0);
		REQUIRE(bb1.bounds[0][3] == 9.0);

		REQUIRE(bb1.bounds[1][0] == -2.5);
		REQUIRE(bb1.bounds[1][1] == -3.5);
		REQUIRE(bb1.bounds[1][2] == 7.25);
		REQUIRE(bb1.bounds[1][3] == 8.25);

		REQUIRE(bb1.bounds[2][0] == -2.0);
		REQUIRE(bb1.bounds[2][1] == 1.0);
		REQUIRE(bb1.bounds[2][2] == 2.0);
		REQUIRE(bb1.bounds[2][3] == 5.0);
	}



	// Tests for ::intersect_ray()
	SECTION("intersect_ray_1") {
		// Simple intersection
		Ray r(Vec3(0.125, -8.0, 0.25), Vec3(0.0, 1.0, 0.0));
		r.finalize();
		BBox2 bb(BBox(Vec3(-1.0, -2.5, -0.5), Vec3(8.0, 7.25, 2.0)),
		         BBox(Vec3(-42.0, -3.5, -1.5), Vec3(9.0, 8.25, 1.0)));

		SIMD::float4 hit_ts;
		bool hit0 = false;
		bool hit1 = false;
		std::tie(hit0, hit1) = bb.intersect_ray(r, &hit_ts);

		REQUIRE(hit0 == true);
		REQUIRE(hit_ts[0] == 5.5);
		REQUIRE(hit1 == true);
		REQUIRE(hit_ts[1] == 4.5);
	}

	// Tests for ::intersect_ray()
	SECTION("intersect_ray_2") {
		// Intersection with negative direction
		Ray r(Vec3(0.125, 12.0, 0.25), Vec3(0.0, -1.0, 0.0));
		r.finalize();
		BBox2 bb(BBox(Vec3(-1.0, -2.5, -0.5), Vec3(8.0, 7.25, 2.0)),
		         BBox(Vec3(-42.0, -3.5, -1.5), Vec3(9.0, 8.25, 1.0)));

		SIMD::float4 hit_ts;
		bool hit0 = false;
		bool hit1 = false;
		std::tie(hit0, hit1) = bb.intersect_ray(r, &hit_ts);

		REQUIRE(hit0 == true);
		REQUIRE(hit_ts[0] == 4.75);
		REQUIRE(hit1 == true);
		REQUIRE(hit_ts[1] == 3.75);
	}

	SECTION("intersect_ray_3") {
		// Simple intersection with unnormalized ray
		Ray r(Vec3(0.125, -8.0, 0.25), Vec3(0.0, 2.0, 0.0));
		//r.finalize(); // Commented to avoid normalization of the ray
		BBox2 bb(BBox(Vec3(-1.0, -2.5, -0.5), Vec3(8.0, 7.25, 2.0)),
		         BBox(Vec3(-42.0, -3.5, -1.5), Vec3(9.0, 8.25, 1.0)));

		SIMD::float4 hit_ts;
		bool hit0 = false;
		bool hit1 = false;
		std::tie(hit0, hit1) = bb.intersect_ray(r, &hit_ts);

		REQUIRE(hit0 == true);
		REQUIRE(hit_ts[0] == 2.75);
		REQUIRE(hit1 == true);
		REQUIRE(hit_ts[1] == 2.25);
	}

	SECTION("intersect_ray_4") {
		// Simple miss
		Ray r(Vec3(-20.0, -8.0, 0.25), Vec3(0.0, 1.0, 0.0));
		r.finalize();
		BBox2 bb(BBox(Vec3(-1.0, -2.5, -0.5), Vec3(8.0, 7.25, 2.0)),
		         BBox(Vec3(-42.0, -3.5, -1.5), Vec3(9.0, 8.25, 1.0)));

		SIMD::float4 hit_ts;
		bool hit0 = false;
		bool hit1 = false;
		std::tie(hit0, hit1) = bb.intersect_ray(r, &hit_ts);

		REQUIRE(hit0 == false);
		REQUIRE(hit1 == true);
		REQUIRE(hit_ts[1] == 4.5);
	}

	SECTION("intersect_ray_5") {
		// Intersection from ray that starts inside the bbox
		Ray r(Vec3(0.0, 0.0, 0.0), Vec3(0, 1.0, 0));
		r.finalize();
		BBox2 bb(BBox(Vec3(-1.0, -2.5, -0.5), Vec3(8.0, 7.25, 2.0)),
		         BBox(Vec3(-42.0, -3.5, -1.5), Vec3(9.0, 8.25, 1.0)));

		SIMD::float4 hit_ts;
		bool hit0 = false;
		bool hit1 = false;
		std::tie(hit0, hit1) = bb.intersect_ray(r, &hit_ts);

		REQUIRE(hit0 == true);
		REQUIRE(hit_ts[0] == 0.0);
		REQUIRE(hit1 == true);
		REQUIRE(hit_ts[1] == 0.0);
	}

	SECTION("intersect_ray_6") {
		// Intersection with collapsed BBox, should be true
		Ray r(Vec3(-4.0, 0.0, 0.0), Vec3(1.0, 0.0, 0.0));
		r.finalize();
		BBox2 bb(BBox(Vec3(1.0, -1.0, -1.0), Vec3(1.0, 1.0, 1.0)),
		         BBox(Vec3(2.0, -2.0, -2.0), Vec3(2.0, 2.0, 2.0)));

		SIMD::float4 hit_ts;
		bool hit0 = false;
		bool hit1 = false;
		std::tie(hit0, hit1) = bb.intersect_ray(r, &hit_ts);

		REQUIRE(hit0 == true);
		REQUIRE(hit_ts[0] == 5.0);
		REQUIRE(hit1 == true);
		REQUIRE(hit_ts[1] == 6.0);
	}
#if 0
	SECTION("intersect_ray_7") {
		// Intersection from ray that grazes the side of the bbox
		Ray r(Vec3(-1.0, -8.0, 0.25), Vec3(0, 1.0, 0));
		r.finalize();
		BBox bb(Vec3(-1.0, -2.5, -0.5), Vec3(8.0, 7.25, 2.0));

		REQUIRE(bb.intersect_ray(r) == false);
	}



#endif

#endif
}

// TODO: - diagonal rays
//       - rays with different tmax values

