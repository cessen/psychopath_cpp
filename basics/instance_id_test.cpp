#include "test.hpp"

#include "instance_id.hpp"


/*
 ************************************************************************
 * Testing suite for InstanceID.
 ************************************************************************
 */

TEST_CASE("InstanceID") {
	// Test for the first constructor
	SECTION("push/pop back") {
		InstanceID id;

		id.push_back(1, 1);
		id.push_back(3, 2);
		id.push_back(63, 10);
		id.push_back(7, 5);

		REQUIRE(id.pop_back(5) == 7);
		REQUIRE(id.pop_back(10) == 63);
		REQUIRE(id.pop_back(2) == 3);
		REQUIRE(id.pop_back(1) == 1);
	}

	// Test for the first constructor
	SECTION("push back, pop front") {
		InstanceID id;

		id.push_back(1, 1);
		id.push_back(3, 2);
		id.push_back(63, 10);
		id.push_back(7, 5);

		REQUIRE(id.pop_front(1) == 1);
		REQUIRE(id.pop_front(2) == 3);
		REQUIRE(id.pop_front(10) == 63);
		REQUIRE(id.pop_front(5) == 7);
	}
}
