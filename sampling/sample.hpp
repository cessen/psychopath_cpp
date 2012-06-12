#ifndef SAMPLE_HPP
#define SAMPLE_HPP

#include "numtype.h"
#include <vector>

struct Sample {
	float32 x, y; // Image plane coordinates
	float32 u, v; // Lens coordinates
	float32 t; // Time coordinate
	std::vector<float32> ns;

	void operator=(const Sample &b) {
		x = b.x;
		y = b.y;
		u = b.u;
		v = b.v;
		t = b.t;

		uint32 s = b.ns.size();
		if (ns.size() != s)
			ns.resize(s);

		for (uint32 i = 0; i < s; i++) {
			ns[i] = b.ns[i];
		}
	}
};

#endif
