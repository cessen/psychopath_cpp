#include "test.hpp"
#include <iostream>

#include "array.hpp"

BOOST_AUTO_TEST_SUITE(array);

// Constructors
BOOST_AUTO_TEST_CASE(constructor_1)
{
	Array<int> ar;

	BOOST_CHECK(ar.size() == 0 && ar.capacity() == 0);
}
BOOST_AUTO_TEST_CASE(constructor_2)
{
	Array<int> ar(100);

	BOOST_CHECK(ar.size() == 100 && ar.capacity() == 100);
}

// Array size manipulations
BOOST_AUTO_TEST_CASE(reserve)
{
	Array<int> ar;

	// Should grow
	ar.reserve(1000);
	BOOST_CHECK(ar.size() == 0 && ar.capacity() == 1000);

	// Capacity should not shrink
	ar.reserve(100);
	BOOST_CHECK(ar.size() == 0 && ar.capacity() == 1000);
}
BOOST_AUTO_TEST_CASE(resize)
{
	Array<int> ar;

	// Should grow
	ar.resize(1000);
	BOOST_CHECK(ar.size() == 1000 && ar.capacity() == 1000);

	// Size should shrink
	ar.resize(100);
	BOOST_CHECK(ar.size() == 100 && ar.capacity() == 1000);
}

// Element access
BOOST_AUTO_TEST_CASE(op_brackets)
{
	Array<int> ar(1000);
	for (int i = 0; i < (int)(ar.size()); i++) {
		ar[i] = i;
	}

	bool eq = true;
	for (int i = 0; i < (int)(ar.size()); i++) {
		eq &= (ar[i] == i);
	}
	BOOST_CHECK(eq);
}

// Keeping values
BOOST_AUTO_TEST_CASE(keep_val)
{
	Array<int> ar(1000);
	for (int i = 0; i < (int)(ar.size()); i++) {
		ar[i] = i;
	}
	ar.resize(100);
	ar.resize(1000);

	bool eq = true;
	for (int i = 0; i < (int)(ar.size()); i++) {
		eq &= (ar[i] == i);
	}
	BOOST_CHECK(eq);
}

// Adding/removing elements
BOOST_AUTO_TEST_CASE(push_back_1)
{
	Array<int> ar;
	for (int i = 0; i < 100; i++) {
		ar.push_back(i);
	}

	bool eq = true;
	for (int i = 0; i < 100; i++) {
		eq &= (ar[i] == i);
	}
	BOOST_CHECK(eq);
	BOOST_CHECK(ar.size() == 100 && ar.capacity() == 100);
}
BOOST_AUTO_TEST_CASE(push_back_2)
{
	Array<int> ar;
	ar.reserve(1000);
	for (int i = 0; i < 100; i++) {
		ar.push_back(i);
	}

	bool eq = true;
	for (int i = 0; i < 100; i++) {
		eq &= (ar[i] == i);
	}
	BOOST_CHECK(eq);
	BOOST_CHECK(ar.size() == 100 && ar.capacity() == 1000);
}

BOOST_AUTO_TEST_SUITE_END();

