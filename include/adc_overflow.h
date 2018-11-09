#ifndef _ADC_OVERFLOW_H_
#define _ADC_OVERFLOW_H_

#include <stdint.h>
#include <stdio.h>

int16_t adcOverflowEnable(int state);
int adcOverflowIsEnabled();
uint16_t adcOverflowGetMin();
void adcOverflowSetMin(uint16_t value);
uint16_t adcOverflowGetMax();
void adcOverflowSetMax(uint16_t value);
int adcOverflowCheck1DCS(uint16_t dcs0);
int adcOverflowCheck2DCS(uint16_t dcs0, uint16_t dcs1);
int adcOverflowCheck4DCS(uint16_t dcs0, uint16_t dcs1, uint16_t dcs2, uint16_t dcs3);

#endif
