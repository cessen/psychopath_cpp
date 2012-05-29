#include "numtype.h"

#include "raster.hpp"

Raster::Raster(int w, int h, int cc, float32 x1, float32 y1, float32 x2, float32 y2)
{
	width = w;
	height = h;
	min_x = x1 < x2 ? x1 : x2;
	min_y = y1 < y2 ? y1 : y2;
	max_x = x1 > x2 ? x1 : x2;
	max_y = y1 > y2 ? y1 : y2;

	channels = cc;
	pixels = new float32[w*h*cc];
	for (int i=0; i < w*h*cc; i++)
		pixels[i] = 0.0;
}

Raster::~Raster()
{
	delete [] pixels;
}

