#ifndef MISC_H
#define MISC_H

#include <stdint.h>

static inline uint64_t
misc_nextpow2(uint64_t n)
{
	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;
	n |= n >> 32;
	return n;
}

#endif
