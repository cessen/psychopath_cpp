#ifndef RNG_H
#define RNG_H

#include <stdint.h>

void init_rand(uint32_t x);
uint32_t rand_cmwc(void);

#endif
