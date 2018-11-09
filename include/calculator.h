#ifndef CALCULATOR_H_
#define CALCULATOR_H_

#include <stdint.h>

struct TofResult{
	int16_t dist;
	int16_t amp;
};

uint16_t calculatorGetPixelMask();
void calculatorSetPixelMask(uint16_t bitMask);
void calculatorInit(uint8_t const divideAmplitudeBy);
void calculatorSetArgs2DCS(uint16_t const dcs0, uint16_t const dcs1, unsigned int const index, unsigned int const indexCalibration);
void calculatorSetArgs4DCS(uint16_t const dcs0, uint16_t const dcs1, uint16_t const dcs2, uint16_t const dcs3, unsigned int const index, unsigned int const indexCalibration);
int16_t calculatorGetAmpSine();
int16_t calculatorGetAmpPN();
struct TofResult calculatorGetDistAndAmpSine();
struct TofResult calculatorGetDistAndAmpPN();

#endif /* CALCULATOR_H_ */
