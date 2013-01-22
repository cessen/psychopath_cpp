#include "test.hpp"
#include "job_queue.hpp"

// Simple callable class that does nothing more than set an integer
// value.
class TestJob
{
	int *inc;
	int value;
public:
	TestJob() {}

	TestJob(int *incr, int val) {
		inc = incr;
		value = val;
	}

	void operator()() {
		*inc = value;
	}
};




BOOST_AUTO_TEST_SUITE(job_queue);

BOOST_AUTO_TEST_CASE(basic_usage)
{
	JobQueue<TestJob> q;
	int ints[100];
	for (int i = 0; i < 100; i++)
		q.push(TestJob(&(ints[i]), i));
	q.finish();

	bool test = true;
	for (int i = 0; i < 100; i++)
		test = test && ints[i] == i;

	BOOST_CHECK(test);
}

BOOST_AUTO_TEST_CASE(queue_bottleneck)
{
	JobQueue<TestJob> q(1000, 2);  // 1000 threads, queue size of 2
	int ints[100];
	for (int i = 0; i < 100; i++)
		q.push(TestJob(&(ints[i]), i));
	q.finish();

	bool test = true;
	for (int i = 0; i < 100; i++)
		test = test && ints[i] == i;

	BOOST_CHECK(test);
}

BOOST_AUTO_TEST_CASE(destruct)
{
	JobQueue<TestJob> *q;
	q = new JobQueue<TestJob>;
	int ints[100];
	for (int i = 0; i < 100; i++)
		q->push(TestJob(&(ints[i]), i));
	delete q; // Should call finish() via destructor

	bool test = true;
	for (int i = 0; i < 100; i++)
		test = test && ints[i] == i;

	BOOST_CHECK(test);
}



BOOST_AUTO_TEST_SUITE_END();

