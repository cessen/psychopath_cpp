#include "test.hpp"

#include <iostream>
#include "vector.hpp"

/*
 ************************************************************************
 * Test suite for Vec3.
 ************************************************************************
 */

TEST_CASE("vector")
{
	// Test for the constructor
	SECTION("constructor") {
		Vec3 v1(0.0, 0.0, 0.0);
		Vec3 v2(1.5, 0.0, -64.0);

		bool t1 = v1.x == 0.0 && v1.y == 0.0 && v1.z == 0.0;
		REQUIRE(t1);
		bool t2 = v2.x == 1.5 && v2.y == 0.0 && v2.z == -64.0;
		REQUIRE(t2);
	}

	// Test for ::operator[]
	SECTION("op_square_bracket") {
		Vec3 v1(1.5, 0.0, -64.0);
		const Vec3 v2(1.5, 0.0, -64.0);

		// Access
		bool t1 = v1[0] == 1.5 && v1[1] == 0.0 && v1[2] == -64.0;
		REQUIRE(t1);
		bool t2 = v2[0] == 1.5 && v2[1] == 0.0 && v2[2] == -64.0;
		REQUIRE(t2);

		// Modification
		v1[0] = 1.0;
		v1[1] = 2.0;
		v1[2] = 3.0;
		bool t3 = v1[0] == 1.0 && v1[1] == 2.0 && v1[2] == 3.0;
		REQUIRE(t3);
	}

	// Test for ::operator+
	SECTION("op_add") {
		Vec3 v1(1.2, -2.6, 1.0);
		Vec3 v2(-23.4, 2.0, 9.0);

		Vec3 v3 = v1 + v2;

		REQUIRE(v3.x == Approx(-22.2).epsilon(0.00001));
		REQUIRE(v3.y == Approx(-0.6).epsilon(0.0001));
		REQUIRE(v3.z == 10.0);
	}

	// Test for ::operator-
	SECTION("op_subtract") {
		Vec3 v1(1.2, -2.6, 1.0);
		Vec3 v2(-23.4, 2.2, 9.0);

		Vec3 v3 = v1 - v2;

		REQUIRE(v3.x == Approx(24.6).epsilon(0.00001));
		REQUIRE(v3.y == Approx(-4.8).epsilon(0.00001));
		REQUIRE(v3.z == -8.0);
	}

	// Test for ::operator*
	SECTION("op_multiply") {
		Vec3 v1(1.2, -2.6, 1.0);

		Vec3 v2 = v1 * 1.5;

		REQUIRE(v2.x == Approx(1.8).epsilon(0.00001));
		REQUIRE(v2.y == Approx(-3.9).epsilon(0.00001));
		REQUIRE(v2.z == 1.5);
	}

	// Test for ::operator/
	SECTION("op_divide") {
		Vec3 v1(1.2, -2.6, 1.0);

		Vec3 v2 = v1 / 1.5;

		REQUIRE(v2.x == Approx(0.8).epsilon(0.00001));
		REQUIRE(v2.y == Approx(-1.7333333333333333333333333).epsilon(0.00001));
		REQUIRE(v2.z == Approx(0.6666666666666666666666666).epsilon(0.00001));
	}

	// Test for ::length()
	SECTION("length") {
		Vec3 v1(1.2, -2.6, 1.0);

		REQUIRE(v1.length() == Approx(3.03315017762062).epsilon(0.0001));
	}

	// Test for ::length2()
	SECTION("length2") {
		Vec3 v1(1.2, -2.6, 1.0);

		REQUIRE(v1.length2() == Approx(9.2).epsilon(0.0001));
	}

	// Test for ::normalize()
	SECTION("normalize") {
		Vec3 v(1.2, -2.6, 1.0);

		float l = v.length();
		v.normalize();

		REQUIRE(l   == Approx(3.03315017762062).epsilon(0.0001));
		REQUIRE(v.x == Approx(0.39562828403747).epsilon(0.0001));
		REQUIRE(v.y == Approx(-0.85719461541452).epsilon(0.0001));
		REQUIRE(v.z == Approx(0.32969023669789).epsilon(0.0001));
	}

	// Test for dot()
	SECTION("dot_") {
		Vec3 v1(1.2, -2.6, 1.0);
		Vec3 v2(-23.4, 2.2, 9.0);

		float d = dot(v1, v2);

		REQUIRE(d == Approx(-24.8).epsilon(0.00001));
	}

	// Test for cross()
	SECTION("cross_") {
		Vec3 v1(1.2, -2.6, 1.0);
		Vec3 v2(-23.4, 2.2, 9.0);

		Vec3 v3 = cross(v1, v2);

		REQUIRE(v3.x == Approx(-25.6).epsilon(0.00001));
		REQUIRE(v3.y == Approx(-34.2).epsilon(0.00001));
		REQUIRE(v3.z == Approx(-58.2).epsilon(0.00001));
	}
}

