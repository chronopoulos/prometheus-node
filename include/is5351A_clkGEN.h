#ifndef _IS5351A_CLKGEN_H_
#define _IS5351A_CLKGEN_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

typedef enum {
	MHZ_24 = 24,
	MHZ_30 = 30,
	MHZ_40 = 40,
	MHZ_36 = 36,
	MHZ_50 = 50
} modFreq;

int16_t initClkGen(int clkMhz);

#endif
