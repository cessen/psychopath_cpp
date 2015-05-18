#include "test.hpp"
#include "ring_buffer.hpp"


TEST_CASE("ring_buffer")
{
	/* constructor tests */
	SECTION("constructor_1") {
		RingBuffer<int> rb;

		REQUIRE(rb.size() == 1);
	}

	SECTION("constructor_2") {
		RingBuffer<int> rb(100);

		REQUIRE(rb.size() == 100);
	}




	/* resize() tests */
	SECTION("resize_1") {
		RingBuffer<int> rb;
		rb.resize(100);

		REQUIRE(rb.size() == 100);
	}

	SECTION("resize_2") {
		RingBuffer<int> rb(50);
		rb.resize(100);

		REQUIRE(rb.size() == 100);
	}




	/* is_empty() tests */
	SECTION("is_empty_1") {
		// No items added
		RingBuffer<int> rb(100);

		REQUIRE(rb.is_empty());
	}

	SECTION("is_empty_2") {
		// A few items added
		RingBuffer<int> rb(100);
		for (int i = 0; i < 5; i++)
			rb.push(i);

		REQUIRE(!rb.is_empty());
	}

	SECTION("is_empty_3") {
		// Max out buffer with items
		RingBuffer<int> rb(100);
		for (int i = 0; i < 100; i++)
			rb.push(i);

		REQUIRE(!rb.is_empty());
	}

	SECTION("is_empty_4") {
		// Overflow buffer with items
		RingBuffer<int> rb(100);
		for (int i = 0; i < 350; i++)
			rb.push(i);

		REQUIRE(!rb.is_empty());
	}

	SECTION("is_empty_5") {
		// Items added and all removed
		RingBuffer<int> rb(100);
		for (int i = 0; i < 50; i++)
			rb.push(i);
		for (int i = 0; i < 50; i++)
			rb.pop();

		REQUIRE(rb.is_empty());
	}

	SECTION("is_empty_6") {
		// Items added and some removed
		RingBuffer<int> rb(100);
		for (int i = 0; i < 50; i++)
			rb.push(i);
		for (int i = 0; i < 25; i++)
			rb.pop();

		REQUIRE(!rb.is_empty());
	}




	/* is_full() tests */
	SECTION("is_full_1") {
		// No items added
		RingBuffer<int> rb(100);

		REQUIRE(!rb.is_full());
	}

	SECTION("is_full_2") {
		// A few items added
		RingBuffer<int> rb(100);
		for (int i = 0; i < 5; i++)
			rb.push(i);

		REQUIRE(!rb.is_full());
	}

	SECTION("is_full_3") {
		// Max out buffer with items
		RingBuffer<int> rb(100);
		for (int i = 0; i < 100; i++)
			rb.push(i);

		REQUIRE(rb.is_full());
	}

	SECTION("is_full_4") {
		// Overflow buffer with items
		RingBuffer<int> rb(100);
		for (int i = 0; i < 350; i++)
			rb.push(i);

		REQUIRE(rb.is_full());
	}

	SECTION("is_full_5") {
		// Items added and all removed
		RingBuffer<int> rb(100);
		for (int i = 0; i < 50; i++)
			rb.push(i);
		for (int i = 0; i < 50; i++)
			rb.pop();

		REQUIRE(!rb.is_full());
	}

	SECTION("is_full_6") {
		// Items added and some removed
		RingBuffer<int> rb(100);
		for (int i = 0; i < 50; i++)
			rb.push(i);
		for (int i = 0; i < 25; i++)
			rb.pop();

		REQUIRE(!rb.is_full());
	}




	/* push()/pop() tests */
	SECTION("push_pop_1") {
		// Partially fill buffer, then empty it
		RingBuffer<int> rb(100);
		bool test = true;
		for (int i = 0; i < 50; i++)
			rb.push(i);
		for (int i = 0; i < 50; i++)
			test = test && (rb.pop() == i);

		REQUIRE(test);
	}

	SECTION("push_pop_2") {
		// Fully fill buffer, then empty it
		RingBuffer<int> rb(100);
		bool test = true;
		for (int i = 0; i < 100; i++)
			rb.push(i);
		for (int i = 0; i < 100; i++)
			test = test && (rb.pop() == i);

		REQUIRE(test);
	}

	SECTION("push_pop_3") {
		// Overflow buffer, then empty it
		RingBuffer<int> rb(100);
		bool test = true;
		for (int i = 0; i < 350; i++)
			rb.push(i);
		for (int i = 250; i < 350; i++)
			test = test && (rb.pop() == i);

		REQUIRE(test);
	}

	SECTION("push_pop_4") {
		// Push and pop repeatedly
		RingBuffer<int> rb(100);
		bool test = true;
		for (int i = 0; i < 350; i++) {
			rb.push(i);
			test = test && (rb.pop() == i);
		}

		REQUIRE(test);
	}

	SECTION("push_pop_5") {
		// Push and pop repeatedly in chunks
		RingBuffer<int> rb(100);
		bool test = true;
		for (int i = 0; i < 350; i++) {
			rb.push(i);
			rb.push(i+1);
			rb.push(i+2);
			rb.push(i+3);
			rb.push(i+4);
			rb.push(i+5);
			test = test && (rb.pop() == i);
			test = test && (rb.pop() == i+1);
			test = test && (rb.pop() == i+2);
			test = test && (rb.pop() == i+3);
			test = test && (rb.pop() == i+4);
			test = test && (rb.pop() == i+5);
		}

		REQUIRE(test);
	}
}



