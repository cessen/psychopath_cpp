#ifndef SAMPLE_H
#define SAMPLE_H

#include "numtype.h"

struct Sample
{
    float32 x, y; // Image plane coordinates
    float32 u, v; // Lens coordinates
    float32 t; // Time coordinate
};

#endif
