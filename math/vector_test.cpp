#include "test.hpp"

#include <iostream>
#include "vector.hpp"

/*
 ************************************************************************
 * Test suite for Vec3.
 ************************************************************************
 */
BOOST_AUTO_TEST_SUITE(vec3)

// Test for the constructor
BOOST_AUTO_TEST_CASE(constructor)
{
	Vec3 v1(0.0, 0.0, 0.0);
	Vec3 v2(1.5, 0.0, -64.0);

	BOOST_CHECK(v1.x == 0.0 && v1.y == 0.0 && v1.z == 0.0);
	BOOST_CHECK(v2.x == 1.5 && v2.y == 0.0 && v2.z == -64.0);
}

// Test for ::operator[]
BOOST_AUTO_TEST_CASE(op_square_bracket)
{
	Vec3 v1(1.5, 0.0, -64.0);
	const Vec3 v2(1.5, 0.0, -64.0);

	// Access
	BOOST_CHECK(v1[0] == 1.5 && v1[1] == 0.0 && v1[2] == -64.0);
	BOOST_CHECK(v2[0] == 1.5 && v2[1] == 0.0 && v2[2] == -64.0);

	// Modification
	v1[0] = 1.0;
	v1[1] = 2.0;
	v1[2] = 3.0;
	BOOST_CHECK(v1[0] == 1.0 && v1[1] == 2.0 && v1[2] == 3.0);
}

// Test for ::operator+
BOOST_AUTO_TEST_CASE(op_add)
{
	Vec3 v1(1.2, -2.6, 1.0);
	Vec3 v2(-23.4, 2.0, 9.0);

	Vec3 v3 = v1 + v2;

	BOOST_CHECK_CLOSE(v3.x, -22.2, 0.00001);
	BOOST_CHECK_CLOSE(v3.y, -0.6,  0.0001);
	BOOST_CHECK_EQUAL(v3.z, 10.0);
}

// Test for ::operator-
BOOST_AUTO_TEST_CASE(op_subtract)
{
	Vec3 v1(1.2, -2.6, 1.0);
	Vec3 v2(-23.4, 2.2, 9.0);

	Vec3 v3 = v1 - v2;

	BOOST_CHECK_CLOSE(v3.x, 24.6, 0.00001);
	BOOST_CHECK_CLOSE(v3.y, -4.8, 0.00001);
	BOOST_CHECK_EQUAL(v3.z, -8.0);
}

// Test for ::operator*
BOOST_AUTO_TEST_CASE(op_multiply)
{
	Vec3 v1(1.2, -2.6, 1.0);

	Vec3 v2 = v1 * 1.5;

	BOOST_CHECK_CLOSE(v2.x, 1.8, 0.00001);
	BOOST_CHECK_CLOSE(v2.y, -3.9, 0.00001);
	BOOST_CHECK_EQUAL(v2.z, 1.5);
}

// Test for ::operator/
BOOST_AUTO_TEST_CASE(op_divide)
{
	Vec3 v1(1.2, -2.6, 1.0);

	Vec3 v2 = v1 / 1.5;

	BOOST_CHECK_CLOSE(v2.x, 0.8, 0.00001);
	BOOST_CHECK_CLOSE(v2.y, -1.7333333333333333333333333, 0.00001);
	BOOST_CHECK_CLOSE(v2.z, 0.6666666666666666666666666, 0.00001);
}

// Test for ::length()
BOOST_AUTO_TEST_CASE(length)
{
	Vec3 v1(1.2, -2.6, 1.0);

	BOOST_CHECK_CLOSE(v1.length(), 3.03315017762062, 0.0001);
}

// Test for ::length2()
BOOST_AUTO_TEST_CASE(length2)
{
	Vec3 v1(1.2, -2.6, 1.0);

	BOOST_CHECK_CLOSE(v1.length2(), 9.2, 0.0001);
}

// Test for ::normalize()
BOOST_AUTO_TEST_CASE(normalize)
{
	Vec3 v(1.2, -2.6, 1.0);

	float l = v.length();
	v.normalize();

	BOOST_CHECK_CLOSE(l,    3.03315017762062, 0.0001);
	BOOST_CHECK_CLOSE(v.x,  0.39562828403747, 0.0001);
	BOOST_CHECK_CLOSE(v.y, -0.85719461541452, 0.0001);
	BOOST_CHECK_CLOSE(v.z,  0.32969023669789, 0.0001);
}

// Test for dot()
BOOST_AUTO_TEST_CASE(dot_)
{
	Vec3 v1(1.2, -2.6, 1.0);
	Vec3 v2(-23.4, 2.2, 9.0);

	float d = dot(v1, v2);

	BOOST_CHECK_CLOSE(d, -24.8, 0.00001);
}

// Test for cross()
BOOST_AUTO_TEST_CASE(cross_)
{
	Vec3 v1(1.2, -2.6, 1.0);
	Vec3 v2(-23.4, 2.2, 9.0);

	Vec3 v3 = cross(v1, v2);

	BOOST_CHECK_CLOSE(v3.x, -25.6, 0.00001);
	BOOST_CHECK_CLOSE(v3.y, -34.2, 0.00001);
	BOOST_CHECK_CLOSE(v3.z, -58.2, 0.00001);
}

BOOST_AUTO_TEST_SUITE_END()

