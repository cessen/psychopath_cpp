#include "test.hpp"
#include <iostream>

#include "simd.hpp"

using namespace SIMD;

BOOST_AUTO_TEST_SUITE(simd_float4);


/* constructor tests */
BOOST_AUTO_TEST_CASE(constructor_1)
{
	float4 f;

	BOOST_CHECK(f[0] == 0.0f && f[1] == 0.0f && f[2] == 0.0f && f[3] == 0.0f);
}

BOOST_AUTO_TEST_CASE(constructor_2)
{
	float4 f(2.0f);

	BOOST_CHECK(f[0] == 2.0f && f[1] == 2.0f && f[2] == 2.0f && f[3] == 2.0f);
}

BOOST_AUTO_TEST_CASE(constructor_3)
{
	float4 f(1.0f, 2.0f, 3.0f, 4.0f);

	BOOST_CHECK(f[0] == 1.0f && f[1] == 2.0f && f[2] == 3.0f && f[3] == 4.0f);
}

BOOST_AUTO_TEST_CASE(constructor_4)
{
	float fs[4] = {1.0f, 2.0f, 3.0f, 4.0f};
	float4 f(fs);

	BOOST_CHECK(f[0] == 1.0f && f[1] == 2.0f && f[2] == 3.0f && f[3] == 4.0f);
}





BOOST_AUTO_TEST_SUITE_END();
