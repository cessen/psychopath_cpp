#include "test.hpp"

#include "chunked_array.hpp"

#define INITIAL_VALUE 123456

struct MyInt {
	int n;
	MyInt() {
		n = INITIAL_VALUE;
	}
};



TEST_CASE("chunked_array")
{
	SECTION("constructor_1") {
		ChunkedArray<int, 10> ar;
		REQUIRE(ar.size() == 0);
	}

	SECTION("constructor_2") {
		ChunkedArray<int, 10> ar(1013);
		REQUIRE(ar.size() == 1013);
	}

	SECTION("constructor_3") {
		ChunkedArray<MyInt, 10> ar(1013);

		bool eq = true;
		for (size_t i = 0; i < ar.size(); i++)
			eq = eq && (ar[i].n == INITIAL_VALUE);

		REQUIRE(eq);
	}

	SECTION("read_write_1") {
		ChunkedArray<int, 10> ar(4011);

		for (size_t i = 0; i < ar.size(); i++)
			ar[i] = i;

		bool eq = true;
		for (size_t i = 0; i < ar.size(); i++)
			eq = eq && (ar[i] == (int)i);

		REQUIRE(eq);
	}

	SECTION("resize_1") {
		ChunkedArray<int, 10> ar;
		ar.resize(1013);

		REQUIRE(ar.size() == 1013);
	}

	SECTION("resize_2") {
		ChunkedArray<MyInt, 10> ar(12);
		ar.resize(1013);

		bool eq = true;
		for (size_t i = 0; i < ar.size(); i++)
			eq = eq && (ar[i].n == INITIAL_VALUE);

		REQUIRE(eq);
		REQUIRE(ar.size() == 1013);
	}

	SECTION("resize_3") {
		ChunkedArray<MyInt, 10> ar(40000);
		ar.resize(1013);

		bool eq = true;
		for (size_t i = 0; i < ar.size(); i++)
			eq = eq && (ar[i].n == INITIAL_VALUE);

		REQUIRE(eq);
		REQUIRE(ar.size() == 1013);
	}

	SECTION("resize_4") {
		ChunkedArray<MyInt, 10> ar(40000);
		ar.resize(0);
		ar.resize(6230);
		ar.resize(10000);
		ar.resize(943);
		ar.resize(302853);
		ar.resize(0);
		ar.resize(1013);

		bool eq = true;
		for (size_t i = 0; i < ar.size(); i++)
			eq = eq && (ar[i].n == INITIAL_VALUE);

		REQUIRE(eq);
		REQUIRE(ar.size() == 1013);
	}

	SECTION("iterator_1") {
		ChunkedArray<int, 10> ar(1234);
		ChunkedArray<int, 10>::iterator it = ar.begin();

		for (size_t i = 0; i < ar.size(); i++)
			it[i] = i;

		bool eq = true;
		it = ar.begin();
		for (size_t i = 0; i < ar.size(); i++)
			eq = eq && ((size_t)it[i] == i);

		REQUIRE(eq);
	}

	SECTION("iterator_2") {
		ChunkedArray<int, 10> ar(1234);
		ChunkedArray<int, 10>::iterator it = ar.begin() + 23;

		for (size_t i = 23; i < ar.size(); i++)
			it[i-23] = i;

		bool eq = true;
		it = ar.begin() + 23;
		for (size_t i = 23; i < ar.size(); i++)
			eq = eq && ((size_t)it[i-23] == i);

		REQUIRE(eq);
	}

	SECTION("iterator_3") {
		ChunkedArray<int, 10> ar(1234);
		ChunkedArray<int, 10>::iterator it = ar.begin() + 23;

		ar[23] = 54321;

		REQUIRE(*it == 54321);
	}

	SECTION("iterator_4") {
		ChunkedArray<int, 10> ar(1234);
		ChunkedArray<int, 10>::iterator it = ar.begin();

		for (size_t i = 0; i < ar.size(); i++)
			ar[i] = i;

		bool eq = true;
		for (size_t i = 0; i < ar.size(); i++) {
			eq = eq && (ar[i] == *it);
			++it;
		}

		REQUIRE(eq);
	}
}



