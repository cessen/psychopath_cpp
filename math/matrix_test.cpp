#include "test.hpp"

#include <limits>

#include "matrix.hpp"

TEST_CASE("matrix44") {
	// Constructor 1 does nothing, so no need to test

	SECTION("constructor_2") {
		Matrix44 m(0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f);
		Matrix44 m2(m);

		REQUIRE(m2[0][0] == 0.0f);
		REQUIRE(m2[0][1] == 1.0f);
		REQUIRE(m2[0][2] == 2.0f);
		REQUIRE(m2[0][3] == 3.0f);

		REQUIRE(m2[1][0] == 4.0f);
		REQUIRE(m2[1][1] == 5.0f);
		REQUIRE(m2[1][2] == 6.0f);
		REQUIRE(m2[1][3] == 7.0f);

		REQUIRE(m2[2][0] == 8.0f);
		REQUIRE(m2[2][1] == 9.0f);
		REQUIRE(m2[2][2] == 10.0f);
		REQUIRE(m2[2][3] == 11.0f);

		REQUIRE(m2[3][0] == 12.0f);
		REQUIRE(m2[3][1] == 13.0f);
		REQUIRE(m2[3][2] == 14.0f);
		REQUIRE(m2[3][3] == 15.0f);
	}

	SECTION("constructor_3") {
		Matrix44 m(0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f);

		REQUIRE(m[0][0] == 0.0f);
		REQUIRE(m[0][1] == 1.0f);
		REQUIRE(m[0][2] == 2.0f);
		REQUIRE(m[0][3] == 3.0f);

		REQUIRE(m[1][0] == 4.0f);
		REQUIRE(m[1][1] == 5.0f);
		REQUIRE(m[1][2] == 6.0f);
		REQUIRE(m[1][3] == 7.0f);

		REQUIRE(m[2][0] == 8.0f);
		REQUIRE(m[2][1] == 9.0f);
		REQUIRE(m[2][2] == 10.0f);
		REQUIRE(m[2][3] == 11.0f);

		REQUIRE(m[3][0] == 12.0f);
		REQUIRE(m[3][1] == 13.0f);
		REQUIRE(m[3][2] == 14.0f);
		REQUIRE(m[3][3] == 15.0f);
	}

	SECTION("copy assignment") {
		Matrix44 m(0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f);
		Matrix44 m2;
		m2 = m;

		REQUIRE(m2[0][0] == 0.0f);
		REQUIRE(m2[0][1] == 1.0f);
		REQUIRE(m2[0][2] == 2.0f);
		REQUIRE(m2[0][3] == 3.0f);

		REQUIRE(m2[1][0] == 4.0f);
		REQUIRE(m2[1][1] == 5.0f);
		REQUIRE(m2[1][2] == 6.0f);
		REQUIRE(m2[1][3] == 7.0f);

		REQUIRE(m2[2][0] == 8.0f);
		REQUIRE(m2[2][1] == 9.0f);
		REQUIRE(m2[2][2] == 10.0f);
		REQUIRE(m2[2][3] == 11.0f);

		REQUIRE(m2[3][0] == 12.0f);
		REQUIRE(m2[3][1] == 13.0f);
		REQUIRE(m2[3][2] == 14.0f);
		REQUIRE(m2[3][3] == 15.0f);
	}

	SECTION("scalar_multiplication_1") {
		Matrix44 m(0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f);
		Matrix44 m2 = m * 2.0f;

		REQUIRE(m2[0][0] == 0.0f);
		REQUIRE(m2[0][1] == 2.0f);
		REQUIRE(m2[0][2] == 4.0f);
		REQUIRE(m2[0][3] == 6.0f);

		REQUIRE(m2[1][0] == 8.0f);
		REQUIRE(m2[1][1] == 10.0f);
		REQUIRE(m2[1][2] == 12.0f);
		REQUIRE(m2[1][3] == 14.0f);

		REQUIRE(m2[2][0] == 16.0f);
		REQUIRE(m2[2][1] == 18.0f);
		REQUIRE(m2[2][2] == 20.0f);
		REQUIRE(m2[2][3] == 22.0f);

		REQUIRE(m2[3][0] == 24.0f);
		REQUIRE(m2[3][1] == 26.0f);
		REQUIRE(m2[3][2] == 28.0f);
		REQUIRE(m2[3][3] == 30.0f);
	}

	SECTION("scalar_multiplication_2") {
		Matrix44 m(0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f);
		m *= 2.0f;

		REQUIRE(m[0][0] == 0.0f);
		REQUIRE(m[0][1] == 2.0f);
		REQUIRE(m[0][2] == 4.0f);
		REQUIRE(m[0][3] == 6.0f);

		REQUIRE(m[1][0] == 8.0f);
		REQUIRE(m[1][1] == 10.0f);
		REQUIRE(m[1][2] == 12.0f);
		REQUIRE(m[1][3] == 14.0f);

		REQUIRE(m[2][0] == 16.0f);
		REQUIRE(m[2][1] == 18.0f);
		REQUIRE(m[2][2] == 20.0f);
		REQUIRE(m[2][3] == 22.0f);

		REQUIRE(m[3][0] == 24.0f);
		REQUIRE(m[3][1] == 26.0f);
		REQUIRE(m[3][2] == 28.0f);
		REQUIRE(m[3][3] == 30.0f);
	}

	SECTION("scalar_division_1") {
		Matrix44 m(0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f);
		Matrix44 m2 = m / 2.0f;

		REQUIRE(m2[0][0] == 0.0f);
		REQUIRE(m2[0][1] == 0.5f);
		REQUIRE(m2[0][2] == 1.0f);
		REQUIRE(m2[0][3] == 1.5f);

		REQUIRE(m2[1][0] == 2.0f);
		REQUIRE(m2[1][1] == 2.5f);
		REQUIRE(m2[1][2] == 3.0f);
		REQUIRE(m2[1][3] == 3.5f);

		REQUIRE(m2[2][0] == 4.0f);
		REQUIRE(m2[2][1] == 4.5f);
		REQUIRE(m2[2][2] == 5.0f);
		REQUIRE(m2[2][3] == 5.5f);

		REQUIRE(m2[3][0] == 6.0f);
		REQUIRE(m2[3][1] == 6.5f);
		REQUIRE(m2[3][2] == 7.0f);
		REQUIRE(m2[3][3] == 7.5f);
	}

	SECTION("scalar_division_2") {
		Matrix44 m(0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f);
		m /= 2.0f;

		REQUIRE(m[0][0] == 0.0f);
		REQUIRE(m[0][1] == 0.5f);
		REQUIRE(m[0][2] == 1.0f);
		REQUIRE(m[0][3] == 1.5f);

		REQUIRE(m[1][0] == 2.0f);
		REQUIRE(m[1][1] == 2.5f);
		REQUIRE(m[1][2] == 3.0f);
		REQUIRE(m[1][3] == 3.5f);

		REQUIRE(m[2][0] == 4.0f);
		REQUIRE(m[2][1] == 4.5f);
		REQUIRE(m[2][2] == 5.0f);
		REQUIRE(m[2][3] == 5.5f);

		REQUIRE(m[3][0] == 6.0f);
		REQUIRE(m[3][1] == 6.5f);
		REQUIRE(m[3][2] == 7.0f);
		REQUIRE(m[3][3] == 7.5f);
	}

	SECTION("add_1") {
		Matrix44 m1(0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f);
		Matrix44 m2(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f);

		Matrix44 m3 = m1 + m2;

		REQUIRE(m3[0][0] == 1.0f);
		REQUIRE(m3[0][1] == 3.0f);
		REQUIRE(m3[0][2] == 5.0f);
		REQUIRE(m3[0][3] == 7.0f);

		REQUIRE(m3[1][0] == 9.0f);
		REQUIRE(m3[1][1] == 11.0f);
		REQUIRE(m3[1][2] == 13.0f);
		REQUIRE(m3[1][3] == 15.0f);

		REQUIRE(m3[2][0] == 17.0f);
		REQUIRE(m3[2][1] == 19.0f);
		REQUIRE(m3[2][2] == 21.0f);
		REQUIRE(m3[2][3] == 23.0f);

		REQUIRE(m3[3][0] == 25.0f);
		REQUIRE(m3[3][1] == 27.0f);
		REQUIRE(m3[3][2] == 29.0f);
		REQUIRE(m3[3][3] == 31.0f);
	}

	SECTION("add_2") {
		Matrix44 m1(0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f);
		Matrix44 m2(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f);

		m1 += m2;

		REQUIRE(m1[0][0] == 1.0f);
		REQUIRE(m1[0][1] == 3.0f);
		REQUIRE(m1[0][2] == 5.0f);
		REQUIRE(m1[0][3] == 7.0f);

		REQUIRE(m1[1][0] == 9.0f);
		REQUIRE(m1[1][1] == 11.0f);
		REQUIRE(m1[1][2] == 13.0f);
		REQUIRE(m1[1][3] == 15.0f);

		REQUIRE(m1[2][0] == 17.0f);
		REQUIRE(m1[2][1] == 19.0f);
		REQUIRE(m1[2][2] == 21.0f);
		REQUIRE(m1[2][3] == 23.0f);

		REQUIRE(m1[3][0] == 25.0f);
		REQUIRE(m1[3][1] == 27.0f);
		REQUIRE(m1[3][2] == 29.0f);
		REQUIRE(m1[3][3] == 31.0f);
	}

	SECTION("subtract_1") {
		Matrix44 m1(0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f);
		Matrix44 m2(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f);

		Matrix44 m3 = m1 - m2;

		REQUIRE(m3[0][0] == -1.0f);
		REQUIRE(m3[0][1] == -1.0f);
		REQUIRE(m3[0][2] == -1.0f);
		REQUIRE(m3[0][3] == -1.0f);

		REQUIRE(m3[1][0] == -1.0f);
		REQUIRE(m3[1][1] == -1.0f);
		REQUIRE(m3[1][2] == -1.0f);
		REQUIRE(m3[1][3] == -1.0f);

		REQUIRE(m3[2][0] == -1.0f);
		REQUIRE(m3[2][1] == -1.0f);
		REQUIRE(m3[2][2] == -1.0f);
		REQUIRE(m3[2][3] == -1.0f);

		REQUIRE(m3[3][0] == -1.0f);
		REQUIRE(m3[3][1] == -1.0f);
		REQUIRE(m3[3][2] == -1.0f);
		REQUIRE(m3[3][3] == -1.0f);
	}

	SECTION("subtract_2") {
		Matrix44 m1(0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f);
		Matrix44 m2(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f);

		m1 -= m2;

		REQUIRE(m1[0][0] == -1.0f);
		REQUIRE(m1[0][1] == -1.0f);
		REQUIRE(m1[0][2] == -1.0f);
		REQUIRE(m1[0][3] == -1.0f);

		REQUIRE(m1[1][0] == -1.0f);
		REQUIRE(m1[1][1] == -1.0f);
		REQUIRE(m1[1][2] == -1.0f);
		REQUIRE(m1[1][3] == -1.0f);

		REQUIRE(m1[2][0] == -1.0f);
		REQUIRE(m1[2][1] == -1.0f);
		REQUIRE(m1[2][2] == -1.0f);
		REQUIRE(m1[2][3] == -1.0f);

		REQUIRE(m1[3][0] == -1.0f);
		REQUIRE(m1[3][1] == -1.0f);
		REQUIRE(m1[3][2] == -1.0f);
		REQUIRE(m1[3][3] == -1.0f);
	}

	SECTION("multiply_1") {
		Matrix44 m1(0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f);
		Matrix44 m2(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f);

		Matrix44 m3 = m1 * m2;

		REQUIRE(m3[0][0] == 62.0f);
		REQUIRE(m3[0][1] == 68.0f);
		REQUIRE(m3[0][2] == 74.0f);
		REQUIRE(m3[0][3] == 80.0f);

		REQUIRE(m3[1][0] == 174.0f);
		REQUIRE(m3[1][1] == 196.0f);
		REQUIRE(m3[1][2] == 218.0f);
		REQUIRE(m3[1][3] == 240.0f);

		REQUIRE(m3[2][0] == 286.0f);
		REQUIRE(m3[2][1] == 324.0f);
		REQUIRE(m3[2][2] == 362.0f);
		REQUIRE(m3[2][3] == 400.0f);

		REQUIRE(m3[3][0] == 398.0f);
		REQUIRE(m3[3][1] == 452.0f);
		REQUIRE(m3[3][2] == 506.0f);
		REQUIRE(m3[3][3] == 560.0f);
	}

	SECTION("multiply_2") {
		Matrix44 m1(0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f);
		Matrix44 m2(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f);

		m1 *= m2;

		REQUIRE(m1[0][0] == 62.0f);
		REQUIRE(m1[0][1] == 68.0f);
		REQUIRE(m1[0][2] == 74.0f);
		REQUIRE(m1[0][3] == 80.0f);

		REQUIRE(m1[1][0] == 174.0f);
		REQUIRE(m1[1][1] == 196.0f);
		REQUIRE(m1[1][2] == 218.0f);
		REQUIRE(m1[1][3] == 240.0f);

		REQUIRE(m1[2][0] == 286.0f);
		REQUIRE(m1[2][1] == 324.0f);
		REQUIRE(m1[2][2] == 362.0f);
		REQUIRE(m1[2][3] == 400.0f);

		REQUIRE(m1[3][0] == 398.0f);
		REQUIRE(m1[3][1] == 452.0f);
		REQUIRE(m1[3][2] == 506.0f);
		REQUIRE(m1[3][3] == 560.0f);
	}

	SECTION("inverse") {
		Matrix44 m1(1.0f, 1.0f, 2.0f, 2.0f, 5.0f, 5.0f, 8.0f, 8.0f, 8.0f, 9.0f, 9.0f, 11.0f, 12.0f, 12.0f, 14.0f, 15.0f);

		Matrix44 m2 = m1.inverse();

		REQUIRE(m2[0][0] == Approx(12.5f).epsilon(0.00001));
		REQUIRE(m2[0][1] == Approx(-5.5f).epsilon(0.00001));
		REQUIRE(m2[0][2] == Approx(-1.0f).epsilon(0.00001));
		REQUIRE(m2[0][3] == Approx(2.0f).epsilon(0.00001));

		REQUIRE(m2[1][0] == Approx(-16.5f).epsilon(0.00001));
		REQUIRE(m2[1][1] == Approx(6.5f).epsilon(0.00001));
		REQUIRE(m2[1][2] == Approx(1.0f).epsilon(0.00001));
		REQUIRE(m2[1][3] == Approx(-2.0f).epsilon(0.00001));

		REQUIRE(m2[2][0] == Approx(-10.5f).epsilon(0.00001));
		REQUIRE(m2[2][1] == Approx(4.5f).epsilon(0.00001));
		REQUIRE(m2[2][2] == Approx(0.0f).epsilon(0.00001));
		REQUIRE(m2[2][3] == Approx(-1.0f).epsilon(0.00001));

		REQUIRE(m2[3][0] == Approx(13.0f).epsilon(0.00001));
		REQUIRE(m2[3][1] == Approx(-5.0f).epsilon(0.00001));
		REQUIRE(m2[3][2] == Approx(0.0f).epsilon(0.00001));
		REQUIRE(m2[3][3] == Approx(1.0f).epsilon(0.00001));
	}

	SECTION("invert") {
		Matrix44 m1(1.0f, 1.0f, 2.0f, 2.0f, 5.0f, 5.0f, 8.0f, 8.0f, 8.0f, 9.0f, 9.0f, 11.0f, 12.0f, 12.0f, 14.0f, 15.0f);

		m1.invert();

		REQUIRE(m1[0][0] == Approx(12.5f).epsilon(0.00001));
		REQUIRE(m1[0][1] == Approx(-5.5f).epsilon(0.00001));
		REQUIRE(m1[0][2] == Approx(-1.0f).epsilon(0.00001));
		REQUIRE(m1[0][3] == Approx(2.0f).epsilon(0.00001));

		REQUIRE(m1[1][0] == Approx(-16.5f).epsilon(0.00001));
		REQUIRE(m1[1][1] == Approx(6.5f).epsilon(0.00001));
		REQUIRE(m1[1][2] == Approx(1.0f).epsilon(0.00001));
		REQUIRE(m1[1][3] == Approx(-2.0f).epsilon(0.00001));

		REQUIRE(m1[2][0] == Approx(-10.5f).epsilon(0.00001));
		REQUIRE(m1[2][1] == Approx(4.5f).epsilon(0.00001));
		REQUIRE(m1[2][2] == Approx(0.0f).epsilon(0.00001));
		REQUIRE(m1[2][3] == Approx(-1.0f).epsilon(0.00001));

		REQUIRE(m1[3][0] == Approx(13.0f).epsilon(0.00001));
		REQUIRE(m1[3][1] == Approx(-5.0f).epsilon(0.00001));
		REQUIRE(m1[3][2] == Approx(0.0f).epsilon(0.00001));
		REQUIRE(m1[3][3] == Approx(1.0f).epsilon(0.00001));
	}

	SECTION("uninvertable") {
		Matrix44 m1(0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f);

		m1.invert();

		REQUIRE(std::isnan(m1[0][0]));
		REQUIRE(std::isnan(m1[0][1]));
		REQUIRE(std::isnan(m1[0][2]));
		REQUIRE(std::isnan(m1[0][3]));

		REQUIRE(std::isnan(m1[1][0]));
		REQUIRE(std::isnan(m1[1][1]));
		REQUIRE(std::isnan(m1[1][2]));
		REQUIRE(std::isnan(m1[1][3]));

		REQUIRE(std::isnan(m1[2][0]));
		REQUIRE(std::isnan(m1[2][1]));
		REQUIRE(std::isnan(m1[2][2]));
		REQUIRE(std::isnan(m1[2][3]));

		REQUIRE(std::isnan(m1[3][0]));
		REQUIRE(std::isnan(m1[3][1]));
		REQUIRE(std::isnan(m1[3][2]));
		REQUIRE(std::isnan(m1[3][3]));
	}

	SECTION("nan") {
		Matrix44 m;
		m.makeNan();

		REQUIRE(std::isnan(m[0][0]));
		REQUIRE(std::isnan(m[0][1]));
		REQUIRE(std::isnan(m[0][2]));
		REQUIRE(std::isnan(m[0][3]));

		REQUIRE(std::isnan(m[1][0]));
		REQUIRE(std::isnan(m[1][1]));
		REQUIRE(std::isnan(m[1][2]));
		REQUIRE(std::isnan(m[1][3]));

		REQUIRE(std::isnan(m[2][0]));
		REQUIRE(std::isnan(m[2][1]));
		REQUIRE(std::isnan(m[2][2]));
		REQUIRE(std::isnan(m[2][3]));

		REQUIRE(std::isnan(m[3][0]));
		REQUIRE(std::isnan(m[3][1]));
		REQUIRE(std::isnan(m[3][2]));
		REQUIRE(std::isnan(m[3][3]));
	}

	SECTION("identity") {
		Matrix44 m;
		m.makeIdentity();

		REQUIRE(m[0][0] == 1.0f);
		REQUIRE(m[0][1] == 0.0f);
		REQUIRE(m[0][2] == 0.0f);
		REQUIRE(m[0][3] == 0.0f);

		REQUIRE(m[1][0] == 0.0f);
		REQUIRE(m[1][1] == 1.0f);
		REQUIRE(m[1][2] == 0.0f);
		REQUIRE(m[1][3] == 0.0f);

		REQUIRE(m[2][0] == 0.0f);
		REQUIRE(m[2][1] == 0.0f);
		REQUIRE(m[2][2] == 1.0f);
		REQUIRE(m[2][3] == 0.0f);

		REQUIRE(m[3][0] == 0.0f);
		REQUIRE(m[3][1] == 0.0f);
		REQUIRE(m[3][2] == 0.0f);
		REQUIRE(m[3][3] == 1.0f);
	}


}