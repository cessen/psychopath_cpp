#include "test.hpp"
#include "ring_buffer_concurrent.hpp"

BOOST_AUTO_TEST_SUITE(ring_buffer_concurrent);


/* constructor tests */
BOOST_AUTO_TEST_CASE(constructor_1)
{
	RingBufferConcurrent<int> rb;

	BOOST_CHECK(rb.size() == 1);
}

BOOST_AUTO_TEST_CASE(constructor_2)
{
	RingBufferConcurrent<int> rb(100);

	BOOST_CHECK(rb.size() == 100);
}




/* resize() tests */
BOOST_AUTO_TEST_CASE(resize_1)
{
	RingBufferConcurrent<int> rb;
	rb.resize(100);

	BOOST_CHECK(rb.size() == 100);
}

BOOST_AUTO_TEST_CASE(resize_2)
{
	RingBufferConcurrent<int> rb(50);
	rb.resize(100);

	BOOST_CHECK(rb.size() == 100);
}




/* push()/pop() tests */
BOOST_AUTO_TEST_CASE(push_pop_1)
{
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

	BOOST_CHECK(test);
}

BOOST_AUTO_TEST_CASE(push_pop_2)
{
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

	BOOST_CHECK(test);
}

BOOST_AUTO_TEST_CASE(push_pop_3)
{
	// Push and pop repeatedly
	RingBufferConcurrent<int> rb(100);
	bool test = true;
	int result {0};
	for (int i = 0; i < 350; i++) {
		rb.push(i);
		rb.pop(&result);
		test = test && (result == i);
	}

	BOOST_CHECK(test);
}

BOOST_AUTO_TEST_CASE(push_pop_4)
{
	// Overflow buffer
	RingBufferConcurrent<int> rb(100);
	bool test = true;
	for (int i = 0; i < 350; i++)
		test = test && rb.push(i); // Should return false when overflowing

	BOOST_CHECK(!test);
}

BOOST_AUTO_TEST_CASE(push_pop_5)
{
	// Overempty buffer
	RingBufferConcurrent<int> rb(100);
	bool test = true;
	for (int i = 0; i < 50; i++)
		rb.push(i);
	int result {0};
	for (int i = 0; i < 60; i++)
		test = test && rb.pop(&result); // Should return false when empty

	BOOST_CHECK(!test);
}







BOOST_AUTO_TEST_SUITE_END();
