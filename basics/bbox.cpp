#include "numtype.h"

#include "bbox.hpp"
#include "utils.hpp"

BBoxT::BBoxT(const int &res_time)
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
	if (bbox.size() != b.bbox.size())
		bbox.init(b.bbox.size());

	for (size_t time=0; time < bbox.size(); time++) {
		bbox[time] = b.bbox[time];
	}
}


