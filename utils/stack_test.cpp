#include "test.hpp"

#include "stack.hpp"

struct alignas(64) Yar {
    int a, b;
};

TEST_CASE("Stack")
{
	SECTION("ints") {
		Stack s(1024, 64);

		auto f = s.push_frame<int>(4);
		f.first[0] = 0;
		f.first[1] = 1;
		f.first[2] = 2;
		f.first[3] = 3;

		f = s.push_frame<int>(4);
		f.first[0] = 4;
		f.first[1] = 5;
		f.first[2] = 6;
		f.first[3] = 7;

		auto tf = s.top_frame<int>();

		REQUIRE(tf.first[0] == 4);
		REQUIRE(tf.first[1] == 5);
		REQUIRE(tf.first[2] == 6);
		REQUIRE(tf.first[3] == 7);

		s.pop_frame();

		tf = s.top_frame<int>();

		REQUIRE(tf.first[0] == 0);
		REQUIRE(tf.first[1] == 1);
		REQUIRE(tf.first[2] == 2);
		REQUIRE(tf.first[3] == 3);

		s.pop_frame();
	}

	SECTION("alignment") {
		Stack s(1024, 64);

		s.push_frame<char>(1);

		auto f = s.push_frame<Yar>(4);

		auto tf = s.top_frame<Yar>();

		REQUIRE((reinterpret_cast<uintptr_t>(f.first) % 64) == 0);
		REQUIRE((reinterpret_cast<uintptr_t>(tf.second) % 64) == 0);
		REQUIRE(&(tf.first[4]) == tf.second);
	}
}