#include "test.hpp"
#include "ring_buffer_concurrent.hpp"



TEST_CASE("ring_buffer_concurrent") {
	/* constructor tests */
	SECTION("constructor_1") {
		RingBufferConcurrent<int> rb;

		REQUIRE(rb.size() == 1);
	}

	SECTION("constructor_2") {
		RingBufferConcurrent<int> rb(100);

		REQUIRE(rb.size() == 100);
	}




	/* resize() tests */
	SECTION("resize_1") {
		RingBufferConcurrent<int> rb;
		rb.resize(100);

		REQUIRE(rb.size() == 100);
	}

	SECTION("resize_2") {
		RingBufferConcurrent<int> rb(50);
		rb.resize(100);

		REQUIRE(rb.size() == 100);
	}




	/* push()/pop() tests */
	SECTION("push_pop_1") {
		// Partially fill buffer, then empty it
		RingBufferConcurrent<int> rb(100);
		bool test = true;
		for (int i = 0; i < 50; i++)
			rb.push(i);
		int result {0};
		for (int i = 0; i < 50; i++) {
			rb.pop(&result);
			test = test && (result == i);
		}

		REQUIRE(test);
	}

	SECTION("push_pop_2") {
		// Fully fill buffer, then empty it
		RingBufferConcurrent<int> rb(100);
		bool test = true;
		for (int i = 0; i < 100; i++)
			rb.push(i);
		int result {0};
		for (int i = 0; i < 100; i++) {
			rb.pop(&result);
			test = test && (result == i);
		}

		REQUIRE(test);
	}

	SECTION("push_pop_3") {
		// Push and pop repeatedly
		RingBufferConcurrent<int> rb(100);
		bool test = true;
		int result {0};
		for (int i = 0; i < 350; i++) {
			rb.push(i);
			rb.pop(&result);
			test = test && (result == i);
		}

		REQUIRE(test);
	}

	SECTION("push_pop_4") {
		// Overflow buffer
		RingBufferConcurrent<int> rb(100);
		bool test = true;
		for (int i = 0; i < 350; i++)
			test = test && rb.push(i); // Should return false when overflowing

		REQUIRE(!test);
	}

	SECTION("push_pop_5") {
		// Overempty buffer
		RingBufferConcurrent<int> rb(100);
		bool test = true;
		for (int i = 0; i < 50; i++)
			rb.push(i);
		int result {0};
		for (int i = 0; i < 60; i++)
			test = test && rb.pop(&result); // Should return false when empty

		REQUIRE(!test);
	}
}







