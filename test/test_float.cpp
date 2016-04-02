#include "test.hpp"

#include <cmath>
#include <limits>


/*
 ************************************************************************
 * Testing suite for floating point values.
 ************************************************************************
 */

TEST_CASE("float") {
	SECTION("inf_1") {
		float yar = std::numeric_limits<float>::infinity();
		float foo = -std::numeric_limits<float>::infinity();

		REQUIRE(std::isinf(yar));
		REQUIRE(std::isinf(foo));
	}

	SECTION("inf_2") {
		float yar1 = 1.0f / 0.0f;
		float yar2 = 1.0f / -0.0f;
		float foo1 = -1.0f / 0.0f;
		float foo2 = -1.0f / -0.0f;

		REQUIRE(std::isinf(yar1));
		REQUIRE(std::isinf(yar2));
		REQUIRE(std::isinf(foo1));
		REQUIRE(std::isinf(foo2));
	}


	SECTION("nan_1") {
		float yar = std::numeric_limits<float>::quiet_NaN();
		float foo = std::numeric_limits<float>::signaling_NaN();

		REQUIRE(std::isnan(yar));
		REQUIRE(std::isnan(foo));
		REQUIRE(yar != yar);
		REQUIRE(foo != foo);
	}

	SECTION("nan_2") {
		float yar1 = 0.0f / 0.0f;
		float yar2 = 0.0f / -0.0f;
		float yar3 = -0.0f / 0.0f;
		float yar4 = -0.0f / -0.0f;

		REQUIRE(std::isnan(yar1));
		REQUIRE(std::isnan(yar2));
		REQUIRE(std::isnan(yar3));
		REQUIRE(std::isnan(yar4));
	}

	SECTION("nan_3") {
		float yar1 = 1.0f + std::numeric_limits<float>::quiet_NaN();
		float yar2 = 1.0f - std::numeric_limits<float>::quiet_NaN();
		float yar3 = 1.0f * std::numeric_limits<float>::quiet_NaN();
		float yar4 = 1.0f / std::numeric_limits<float>::quiet_NaN();

		REQUIRE(std::isnan(yar1));
		REQUIRE(std::isnan(yar2));
		REQUIRE(std::isnan(yar3));
		REQUIRE(std::isnan(yar4));
	}
}


