#include "test.hpp"
#include <iostream>

#include "simd.hpp"

using namespace SIMD;


TEST_CASE("simd") {
	/* constructor tests */
	SECTION("constructor_1") {
		float4 f;

		REQUIRE(f[0] == 0.0f);
		REQUIRE(f[1] == 0.0f);
		REQUIRE(f[2] == 0.0f);
		REQUIRE(f[3] == 0.0f);
	}

	SECTION("constructor_2") {
		float4 f(2.0f);

		REQUIRE(f[0] == 2.0f);
		REQUIRE(f[1] == 2.0f);
		REQUIRE(f[2] == 2.0f);
		REQUIRE(f[3] == 2.0f);
	}

	SECTION("constructor_3") {
		float4 f(1.0f, 2.0f, 3.0f, 4.0f);

		REQUIRE(f[0] == 1.0f);
		REQUIRE(f[1] == 2.0f);
		REQUIRE(f[2] == 3.0f);
		REQUIRE(f[3] == 4.0f);
	}

	SECTION("constructor_4") {
		float fs[4] = {1.0f, 2.0f, 3.0f, 4.0f};
		float4 f(fs);

		REQUIRE(f[0] == 1.0f);
		REQUIRE(f[1] == 2.0f);
		REQUIRE(f[2] == 3.0f);
		REQUIRE(f[3] == 4.0f);
	}
}
