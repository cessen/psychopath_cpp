#include "test.hpp"
#include "vector.hpp"
#include "bbox.hpp"
#include "utils.hpp"


/*
 * Testing suite for BBox.
 */
BOOST_AUTO_TEST_SUITE(bounding_box_t_suite)


BOOST_AUTO_TEST_CASE(


BOOST_AUTO_TEST_SUITE_END()

/*
 * Testing suite for BBoxT.
 */
BOOST_AUTO_TEST_SUITE(bounding_box_t_suite)


// Tests for first constructor
BOOST_AUTO_TEST_CASE(constructor_1) {
	BBoxT bb(4);
	
	BOOST_CHECK(bb.bbox.state_count == 4);
}


// Tests for second constructor
BOOST_AUTO_TEST_CASE(constructor_2) {
	BBoxT bb(Vec3(-1.0, 0.0, -2.0),
	          Vec3(2.0, 1.0, 1.5));

	BOOST_CHECK(bb[0].min == Vec3(-1.0, 0.0, -2.0));
	BOOST_CHECK(bb[0].max == Vec3(2.0, 1.0, 1.5));
}


// Tests for ::init()
BOOST_AUTO_TEST_CASE(init) {
	BBoxT bb(2);
	
	bb.init(8);
	
	BOOST_CHECK(bb.bbox.state_count == 8);
}


// Tests for ::add_time_sample()
BOOST_AUTO_TEST_CASE(add_time_sample) {
	BBoxT bb(4);
	
	bb.add_time_sample(0, Vec3(1.0, 2.0, 3.0), Vec3(2.0, 3.0, 4.0));
	bb.add_time_sample(1, Vec3(4.0, 5.0, 6.0), Vec3(5.0, 6.0, 7.0));
	bb.add_time_sample(2, Vec3(7.0, 8.0, 9.0), Vec3(8.0, 9.0, 10.0));
	bb.add_time_sample(3, Vec3(10.0, 11.0, 12.0), Vec3(11.0, 12.0, 13.0));
	
	BOOST_CHECK(bb[0].min == Vec3(1.0, 2.0, 3.0));
	BOOST_CHECK(bb[0].max == Vec3(2.0, 3.0, 4.0));
	BOOST_CHECK(bb[1].min == Vec3(4.0, 5.0, 6.0));
	BOOST_CHECK(bb[1].max == Vec3(5.0, 6.0, 7.0));
	BOOST_CHECK(bb[2].min == Vec3(7.0, 8.0, 9.0));
	BOOST_CHECK(bb[2].max == Vec3(8.0, 9.0, 10.0));
	BOOST_CHECK(bb[3].min == Vec3(10.0, 11.0, 12.0));
	BOOST_CHECK(bb[3].max == Vec3(11.0, 12.0, 13.0));
}


// Tests for ::at_time()
BOOST_AUTO_TEST_CASE(at_time_1) {
	// BBoxT with two time samples
	BBoxT bb(2);
	bb.add_time_sample(0, Vec3(0.0, 2.0, 3.0), Vec3(2.0, 3.0, 5.0));
	bb.add_time_sample(1, Vec3(4.0, 3.0, 6.0), Vec3(5.0, 6.0, 7.0));
	
	BBox t1 = bb.at_time(0.0);
	BBox t2 = bb.at_time(0.5);
	BBox t3 = bb.at_time(1.0);
	
	BOOST_CHECK(t1.min == Vec3(0.0, 2.0, 3.0));
	BOOST_CHECK(t1.max == Vec3(2.0, 3.0, 5.0));
	BOOST_CHECK(t2.min == Vec3(2.0, 2.5, 4.5));
	BOOST_CHECK(t2.max == Vec3(3.5, 4.5, 6.0));
	BOOST_CHECK(t3.min == Vec3(4.0, 3.0, 6.0));
	BOOST_CHECK(t3.max == Vec3(5.0, 6.0, 7.0));
}

BOOST_AUTO_TEST_CASE(at_time_2) {
	// BBoxT with four time samples
	BBoxT bb(4);
	bb.add_time_sample(0, Vec3(0.0, 2.0, 3.0), Vec3(2.0, 3.0, 5.0));
	bb.add_time_sample(1, Vec3(4.0, 3.0, 6.0), Vec3(5.0, 6.0, 7.0));
	bb.add_time_sample(2, Vec3(7.0, 8.0, 9.0), Vec3(8.0, 9.0, 10.0));
	bb.add_time_sample(3, Vec3(10.0, 11.0, 12.0), Vec3(11.0, 12.0, 13.0));
	
	BBox t1 = bb.at_time(0.0);
	BBox t2 = bb.at_time(0.5);
	BBox t3 = bb.at_time(1.0);
	
	BOOST_CHECK(t1.min == Vec3(0.0, 2.0, 3.0));
	BOOST_CHECK(t1.max == Vec3(2.0, 3.0, 5.0));
	BOOST_CHECK(t2.min == Vec3(5.5, 5.5, 7.5));
	BOOST_CHECK(t2.max == Vec3(6.5, 7.5, 8.5));
	BOOST_CHECK(t3.min == Vec3(10.0, 11.0, 12.0));
	BOOST_CHECK(t3.max == Vec3(11.0, 12.0, 13.0));
}


// Tests for ::copy()
BOOST_AUTO_TEST_CASE(copy_1) {
	BBoxT bb1(Vec3(0,1,2), Vec3(3,5,6));
	BBoxT bb2(2);
	bb2.add_time_sample(0, Vec3(5,2,5), Vec3(9,8,7));
	bb2.add_time_sample(0, Vec3(1,2,3), Vec3(4,3,8));
	
	bb1.copy(bb2);
	
	BOOST_CHECK(bb1[0].min == bb2[0].min);
	BOOST_CHECK(bb1[0].max == bb2[0].max);
	BOOST_CHECK(bb1[1].min == bb2[1].min);
	BOOST_CHECK(bb1[1].max == bb2[1].max);
	BOOST_CHECK(bb1.bbox.state_count == 2);
}

BOOST_AUTO_TEST_CASE(copy_2) {
	BBoxT bb1(2);
	bb1.add_time_sample(0, Vec3(5,2,5), Vec3(9,8,7));
	bb1.add_time_sample(0, Vec3(1,2,3), Vec3(4,3,8));
	BBoxT bb2(Vec3(0,1,2), Vec3(3,5,6));
		
	bb1.copy(bb2);
	
	BOOST_CHECK(bb1[0].min == bb2[0].min);
	BOOST_CHECK(bb1[0].max == bb2[0].max);
	BOOST_CHECK(bb1.bbox.state_count == 1);
}


// Tests for ::merge_with()
BOOST_AUTO_TEST_CASE(merge_with_1) {
	// Test for when state count is equal between bb1 and bb2
	BBoxT bb1(2);
	bb1.add_time_sample(0, Vec3(-2,-3,-5), Vec3(9,8,7));
	bb1.add_time_sample(1, Vec3(-1,-6,-3), Vec3(4,3,8));
	BBoxT bb2(2);
	bb2.add_time_sample(0, Vec3(-4,-2,-1), Vec3(4,2,9));
	bb2.add_time_sample(1, Vec3(4,-5,-3), Vec3(5,3,12));
	
	bb1.merge_with(bb2);
	
	BOOST_CHECK(bb1[0].min == Vec3(-4,-3,-5));
	BOOST_CHECK(bb1[0].max == Vec3(9,8,9));
	BOOST_CHECK(bb1[1].min == Vec3(-1,-6,-3));
	BOOST_CHECK(bb1[1].max == Vec3(5,3,12));
	BOOST_CHECK(bb1.bbox.state_count == 2);
}

BOOST_AUTO_TEST_CASE(merge_with_2) {
	// Test for when bb1 has state count less than bb2
	BBoxT bb1(1);
	bb1.add_time_sample(0, Vec3(-2,-3,-5), Vec3(9,8,7));
	BBoxT bb2(2);
	bb2.add_time_sample(0, Vec3(-4,-2,-1), Vec3(4,2,9));
	bb2.add_time_sample(1, Vec3(4,-5,-3), Vec3(5,3,12));
	
	bb1.merge_with(bb2);
	
	BOOST_CHECK(bb1[0].min == Vec3(-4,-5,-5));
	BOOST_CHECK(bb1[0].max == Vec3(9,8,12));
	BOOST_CHECK(bb1.bbox.state_count == 1);
}

BOOST_AUTO_TEST_CASE(merge_with_3) {
	// Test for when bb1 has state count greater than bb2
	BBoxT bb1(2);
	bb1.add_time_sample(0, Vec3(-2,-3,-5), Vec3(9,8,7));
	bb1.add_time_sample(1, Vec3(-1,-6,-3), Vec3(4,3,8));
	BBoxT bb2(1);
	bb2.add_time_sample(0, Vec3(-4,-2,-1), Vec3(4,2,9));
	
	bb1.merge_with(bb2);
	
	BOOST_CHECK(bb1[0].min == Vec3(-4,-6,-5));
	BOOST_CHECK(bb1[0].max == Vec3(9,8,9));
	BOOST_CHECK(bb1.bbox.state_count == 1);
}

BOOST_AUTO_TEST_SUITE_END()

