#include "test.hpp"

#include "chunked_array.hpp"

#define INITIAL_VALUE 123456

struct MyInt {
	int n;
	MyInt() {
		n = INITIAL_VALUE;
	}
};


BOOST_AUTO_TEST_SUITE(chunked_array);


BOOST_AUTO_TEST_CASE(constructor_1)
{
	ChunkedArray<int, 10> ar;
	BOOST_CHECK(ar.size() == 0);
}

BOOST_AUTO_TEST_CASE(constructor_2)
{
	ChunkedArray<int, 10> ar(1013);
	BOOST_CHECK(ar.size() == 1013);
}

BOOST_AUTO_TEST_CASE(constructor_3)
{
	ChunkedArray<MyInt, 10> ar(1013);

	bool eq = true;
	for (size_t i = 0; i < ar.size(); i++)
		eq = eq && (ar[i].n == INITIAL_VALUE);

	BOOST_CHECK(eq);
}

BOOST_AUTO_TEST_CASE(read_write_1)
{
	ChunkedArray<int, 10> ar(4011);

	for (size_t i = 0; i < ar.size(); i++)
		ar[i] = i;

	bool eq = true;
	for (size_t i = 0; i < ar.size(); i++)
		eq = eq && (ar[i] == (int)i);

	BOOST_CHECK(eq);
}

BOOST_AUTO_TEST_CASE(resize_1)
{
	ChunkedArray<int, 10> ar;
	ar.resize(1013);

	BOOST_CHECK(ar.size() == 1013);
}

BOOST_AUTO_TEST_CASE(resize_2)
{
	ChunkedArray<MyInt, 10> ar(12);
	ar.resize(1013);

	bool eq = true;
	for (size_t i = 0; i < ar.size(); i++)
		eq = eq && (ar[i].n == INITIAL_VALUE);

	BOOST_CHECK(eq);
	BOOST_CHECK(ar.size() == 1013);
}

BOOST_AUTO_TEST_CASE(resize_3)
{
	ChunkedArray<MyInt, 10> ar(40000);
	ar.resize(1013);

	bool eq = true;
	for (size_t i = 0; i < ar.size(); i++)
		eq = eq && (ar[i].n == INITIAL_VALUE);

	BOOST_CHECK(eq);
	BOOST_CHECK(ar.size() == 1013);
}

BOOST_AUTO_TEST_CASE(resize_4)
{
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

	BOOST_CHECK(eq);
	BOOST_CHECK(ar.size() == 1013);
}

BOOST_AUTO_TEST_CASE(iterator_1)
{
	ChunkedArray<int, 10> ar(1234);
	ChunkedArray<int, 10>::iterator it = ar.begin();

	for (size_t i = 0; i < ar.size(); i++)
		it[i] = i;

	bool eq = true;
	it = ar.begin();
	for (size_t i = 0; i < ar.size(); i++)
		eq = eq && ((size_t)it[i] == i);

	BOOST_CHECK(eq);
}

BOOST_AUTO_TEST_CASE(iterator_2)
{
	ChunkedArray<int, 10> ar(1234);
	ChunkedArray<int, 10>::iterator it = ar.begin() + 23;

	for (size_t i = 23; i < ar.size(); i++)
		it[i-23] = i;

	bool eq = true;
	it = ar.begin() + 23;
	for (size_t i = 23; i < ar.size(); i++)
		eq = eq && ((size_t)it[i-23] == i);

	BOOST_CHECK(eq);
}

BOOST_AUTO_TEST_CASE(iterator_3)
{
	ChunkedArray<int, 10> ar(1234);
	ChunkedArray<int, 10>::iterator it = ar.begin() + 23;

	ar[23] = 54321;

	BOOST_CHECK(*it == 54321);
}

BOOST_AUTO_TEST_CASE(iterator_4)
{
	ChunkedArray<int, 10> ar(1234);
	ChunkedArray<int, 10>::iterator it = ar.begin();

	for (size_t i = 0; i < ar.size(); i++)
		ar[i] = i;

	bool eq = true;
	for (size_t i = 0; i < ar.size(); i++) {
		eq = eq && (ar[i] == *it);
		++it;
	}

	BOOST_CHECK(eq);
}


BOOST_AUTO_TEST_SUITE_END();

