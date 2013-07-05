#include "test.hpp"

#include <cmath>
#include <limits>


/*
 ************************************************************************
 * Testing suite for floating point values.
 ************************************************************************
 */
BOOST_AUTO_TEST_SUITE(floating_point)


BOOST_AUTO_TEST_CASE(inf_1)
{
	float yar = std::numeric_limits<float>::infinity();
	float foo = -std::numeric_limits<float>::infinity();

	BOOST_CHECK(std::isinf(yar));
	BOOST_CHECK(std::isinf(foo));
}

BOOST_AUTO_TEST_CASE(inf_2)
{
	float yar1 = 1.0f / 0.0f;
	float yar2 = 1.0f / -0.0f;
	float foo1 = -1.0f / 0.0f;
	float foo2 = -1.0f / -0.0f;

	BOOST_CHECK(std::isinf(yar1));
	BOOST_CHECK(std::isinf(yar2));
	BOOST_CHECK(std::isinf(foo1));
	BOOST_CHECK(std::isinf(foo2));
}


BOOST_AUTO_TEST_CASE(nan_1)
{
	float yar = std::numeric_limits<float>::quiet_NaN();
	float foo = std::numeric_limits<float>::signaling_NaN();

	BOOST_CHECK(std::isnan(yar));
	BOOST_CHECK(std::isnan(foo));
	BOOST_CHECK(yar != yar);
	BOOST_CHECK(foo != foo);
}

BOOST_AUTO_TEST_CASE(nan_2)
{
	float yar1 = 0.0f / 0.0f;
	float yar2 = 0.0f / -0.0f;
	float yar3 = -0.0f / 0.0f;
	float yar4 = -0.0f / -0.0f;

	BOOST_CHECK(std::isnan(yar1));
	BOOST_CHECK(std::isnan(yar2));
	BOOST_CHECK(std::isnan(yar3));
	BOOST_CHECK(std::isnan(yar4));
}

BOOST_AUTO_TEST_CASE(nan_3)
{
	float yar1 = 1.0f + std::numeric_limits<float>::quiet_NaN();
	float yar2 = 1.0f - std::numeric_limits<float>::quiet_NaN();
	float yar3 = 1.0f * std::numeric_limits<float>::quiet_NaN();
	float yar4 = 1.0f / std::numeric_limits<float>::quiet_NaN();

	BOOST_CHECK(std::isnan(yar1));
	BOOST_CHECK(std::isnan(yar2));
	BOOST_CHECK(std::isnan(yar3));
	BOOST_CHECK(std::isnan(yar4));
}




BOOST_AUTO_TEST_SUITE_END()