#ifndef NTT_H
#define NTT_H

#include <stdint.h>

void ntt(uint16_t* poly, int protection_level);
void invntt(uint16_t* poly, int protection_level);

#endif
