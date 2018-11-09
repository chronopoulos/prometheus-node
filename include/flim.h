#ifndef _FLIM_H_
#define _FLIM_H_

#include <stdint.h>

typedef enum {
	SKIP_FLIM_FLASH_WIDTH = 1,
	SKIP_FLIM_FLASH_DELAY = 2,
	SKIP_FLIM_T1 = 3,
	SKIP_FLIM_T2 = 4,
	SKIP_FLIM_T3 = 5,
	SKIP_FLIM_T4 = 6,
	SKIP_FLIM_TREP = 7
} flimModeCounterSkipBit;

int16_t  flimSetT1(uint16_t value);
int16_t  flimSetT2(uint16_t value);
int16_t  flimSetT3(uint16_t value);
int16_t  flimSetT4(uint16_t value);
int16_t  flimSetTREP(uint16_t value);
int16_t  flimSetRepetitions(uint16_t value);
uint16_t flimGetRepetitions();
int16_t  flimSetFlashDelay(uint16_t value);
int16_t  flimSetFlashWidth(uint16_t value);
uint16_t flimGetFlashWidth();

int16_t flimSetDelay(flimModeCounterSkipBit bit, uint16_t value);
void flimSetSkip(flimModeCounterSkipBit bit);
void flimResetSkip(flimModeCounterSkipBit bit);
int16_t flimGetStep();

#endif
