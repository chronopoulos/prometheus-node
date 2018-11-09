#ifndef _SATURATION_H_
#define _SATURATION_H_

#include <stdint.h>
#include <stdio.h>

int16_t saturationEnable(int state);
void saturationSetSaturationBit(uint16_t bitMask);
void saturationSet1DCS(uint16_t dcs0);
void saturationSet2DCS(uint16_t dcs0, uint16_t dcs1);
void saturationSet4DCS(uint16_t dcs0, uint16_t dcs1, uint16_t dcs2, uint16_t dcs3);
int saturationCheck();

#endif
