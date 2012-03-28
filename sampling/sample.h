#ifndef SAMPLE_H
#define SAMPLE_H

struct Sample
{
    float x, y; // Image plane coordinates
    float u, v; // Lens coordinates
    float t; // Time coordinate
};

#endif
