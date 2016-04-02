#include "test.hpp"

#include "rng.hpp"

/*
 * Test suite for the random number generator.
 */
TEST_CASE("RNG") {
	// Test to see if the RNG gives consistent results
	// when given the same seed
	SECTION("consistent") {
		RNG rng1;
		RNG rng2;
		bool equals = true;

		rng1.seed(42);
		rng2.seed(42);
		for (int i = 0; i < 100000; i++) {
			equals = equals && (rng1.next_uint() == rng2.next_uint());
			equals = equals && (rng1.next_float() == rng2.next_float());
		}

		REQUIRE(equals);
	}



	// Test to see if factory-spawned RNG's get different seeds
	SECTION("factory_seed_1") {
		RNG rng1;
		RNG rng2;
		bool equals = true;

		for (int i = 0; i < 4; i++) {
			equals = equals && (rng1.next_uint() == rng2.next_uint());
			equals = equals && (rng1.next_float() == rng2.next_float());
		}

		REQUIRE(!equals);
	}

	// Test to see if the factory code works properly inside object
	// initializations
	class RNGTest {
	public:
		RNG rng;
		RNGTest() {}
	};

	SECTION("factory_seed_2") {
		RNGTest rng1;
		RNGTest rng2;
		bool equals = true;

		for (int i = 0; i < 4; i++) {
			equals = equals && (rng1.rng.next_uint() == rng2.rng.next_uint());
			equals = equals && (rng1.rng.next_float() == rng2.rng.next_float());
		}

		REQUIRE(!equals);
	}
}

