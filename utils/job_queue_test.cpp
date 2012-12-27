#include "test.hpp"
#include "job_queue.hpp"

BOOST_AUTO_TEST_SUITE(job_queue);


BOOST_AUTO_TEST_CASE(enqueue_1)
{
	JobQueue<int> q(100);

	for (int i = 0; i < 100; i++)
		q.push(i);
	q.close();

	bool test = true;
	int n = 0;
	for (int i = 0; i < 100; i++) {
		q.pop(&n);
		test = test && n == i;
	}

	BOOST_CHECK(test);
}



BOOST_AUTO_TEST_SUITE_END();

