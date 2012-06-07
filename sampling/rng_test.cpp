#include "test.hpp"

#include "rng.hpp"

/*
 * Test suite for the random number generator.
 */
BOOST_AUTO_TEST_SUITE(rng);

// Test to see if the RNG gives consistent results
// when given the same seed
BOOST_AUTO_TEST_CASE(consistent)
{
	RNG rng1;
	RNG rng2;
	bool equals = true;

	rng1.seed(42);
	rng2.seed(42);
	for (int i = 0; i < 100000; i++) {
		equals = equals && (rng1.next_uint() == rng2.next_uint());
		equals = equals && (rng1.next_float() == rng2.next_float());
	}

	BOOST_CHECK(equals);
}

BOOST_AUTO_TEST_SUITE_END();

