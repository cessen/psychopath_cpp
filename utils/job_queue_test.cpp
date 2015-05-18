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




TEST_CASE("job_queue")
{
	SECTION("basic_usage") {
		JobQueue<TestJob> q;
		int ints[100];
		for (int i = 0; i < 100; i++)
			q.push(TestJob(&(ints[i]), i));
		q.finish();

		bool test = true;
		for (int i = 0; i < 100; i++)
			test = test && ints[i] == i;

		REQUIRE(test);
	}

	SECTION("queue_bottleneck") {
		JobQueue<TestJob> q(1000, 2);  // 1000 threads, queue size of 2
		int ints[100];
		for (int i = 0; i < 100; i++)
			q.push(TestJob(&(ints[i]), i));
		q.finish();

		bool test = true;
		for (int i = 0; i < 100; i++)
			test = test && ints[i] == i;

		REQUIRE(test);
	}

	SECTION("destruct") {
		JobQueue<TestJob> *q;
		q = new JobQueue<TestJob>;
		int ints[100];
		for (int i = 0; i < 100; i++)
			q->push(TestJob(&(ints[i]), i));
		delete q; // Should call finish() via destructor

		bool test = true;
		for (int i = 0; i < 100; i++)
			test = test && ints[i] == i;

		REQUIRE(test);
	}
}



