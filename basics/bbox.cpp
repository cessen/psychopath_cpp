#include "numtype.h"

#include "bbox.hpp"
#include "utils.hpp"

BBoxT::BBoxT(const int32 &res_time)
{
	bbox.init(res_time);
}


BBoxT::BBoxT(const Vec3 &bmin_, const Vec3 &bmax_)
{
	bbox.init(1);
	bbox[0].min = bmin_;
	bbox[0].max = bmax_;
}


void BBoxT::copy(const BBoxT &b)
{
	if (bbox.state_count != b.bbox.state_count)
		bbox.init(b.bbox.state_count);

	for (int32 time=0; time < bbox.state_count; time++) {
		bbox[time] = b.bbox[time];
	}
}


