#include "test.hpp"

#include <vector>
#include "utils.hpp"

TEST_CASE("mutable_partition")
{
	SECTION("already_partitioned") {
		std::vector<int> v {1, 1, 1, 1, 2, 2, 2, 2};

		auto p = mutable_partition(v.begin(), v.end(), [](int& i) {
			return i == 1;
		});

		REQUIRE(p == v.begin() + 4);
		REQUIRE(v[0] == 1);
		REQUIRE(v[1] == 1);
		REQUIRE(v[2] == 1);
		REQUIRE(v[3] == 1);
		REQUIRE(v[4] == 2);
		REQUIRE(v[5] == 2);
		REQUIRE(v[6] == 2);
		REQUIRE(v[7] == 2);
	}

	SECTION("reverse") {
		std::vector<int> v {2, 2, 2, 2, 1, 1, 1, 1};

		auto p = mutable_partition(v.begin(), v.end(), [](int& i) {
			return i == 1;
		});

		REQUIRE(p == v.begin() + 4);
		REQUIRE(v[0] == 1);
		REQUIRE(v[1] == 1);
		REQUIRE(v[2] == 1);
		REQUIRE(v[3] == 1);
		REQUIRE(v[4] == 2);
		REQUIRE(v[5] == 2);
		REQUIRE(v[6] == 2);
		REQUIRE(v[7] == 2);
	}

	SECTION("interleaved") {
		std::vector<int> v {2, 1, 2, 1, 2, 1, 2, 1};

		auto p = mutable_partition(v.begin(), v.end(), [](int& i) {
			return i == 1;
		});

		REQUIRE(p == v.begin() + 4);
		REQUIRE(v[0] == 1);
		REQUIRE(v[1] == 1);
		REQUIRE(v[2] == 1);
		REQUIRE(v[3] == 1);
		REQUIRE(v[4] == 2);
		REQUIRE(v[5] == 2);
		REQUIRE(v[6] == 2);
		REQUIRE(v[7] == 2);
	}

	SECTION("all_true") {
		std::vector<int> v {1, 1, 1, 1, 1, 1, 1, 1};

		auto p = mutable_partition(v.begin(), v.end(), [](int& i) {
			return i == 1;
		});

		REQUIRE(p == v.end());
		REQUIRE(v[0] == 1);
		REQUIRE(v[1] == 1);
		REQUIRE(v[2] == 1);
		REQUIRE(v[3] == 1);
		REQUIRE(v[4] == 1);
		REQUIRE(v[5] == 1);
		REQUIRE(v[6] == 1);
		REQUIRE(v[7] == 1);
	}

	SECTION("all_false") {
		std::vector<int> v {2, 2, 2, 2, 2, 2, 2, 2};

		auto p = mutable_partition(v.begin(), v.end(), [](int& i) {
			return i == 1;
		});

		REQUIRE(p == v.begin());
		REQUIRE(v[0] == 2);
		REQUIRE(v[1] == 2);
		REQUIRE(v[2] == 2);
		REQUIRE(v[3] == 2);
		REQUIRE(v[4] == 2);
		REQUIRE(v[5] == 2);
		REQUIRE(v[6] == 2);
		REQUIRE(v[7] == 2);
	}

	SECTION("predicate_run_once_per_element") {
		std::vector<int> v {2, 1, 2, 1, 2, 1, 2, 1};
		int n = 0;

		auto p = mutable_partition(v.begin(), v.end(), [&](int& i) {
			++n;
			return i == 1;
		});

		REQUIRE(n == 8);
		REQUIRE(p == v.begin() + 4);
		REQUIRE(v[0] == 1);
		REQUIRE(v[1] == 1);
		REQUIRE(v[2] == 1);
		REQUIRE(v[3] == 1);
		REQUIRE(v[4] == 2);
		REQUIRE(v[5] == 2);
		REQUIRE(v[6] == 2);
		REQUIRE(v[7] == 2);
	}
}