#ifndef _READ_OUT_H_
#define _READ_OUT_H_

#include <stdint.h>

void readOutEnableHorizontalBinning(const int state, const int deviceAddress);
void readOutEnableVerticalBinning(const int state, const int deviceAddress);
void readOutSetRowReduction(const int reducedBy, const int deviceAddress);
void readOutEnableDualMGX(const int state, const int deviceAddress);
void readOutEnableHDR(const int state, const int deviceAddress);
void readOutSetROI(const uint16_t topLeftX, const uint16_t bottomRightX, const uint8_t topLeftY, const uint8_t bottomRightY, const int deviceAddress);
uint8_t readOutIsVerticalBinning(const int deviceAddress);
uint8_t readOutIsHorizontalBinning(const int deviceAddress);
uint16_t readOutGetTopLeftX();
uint16_t readOutGetBottomRightX();
uint8_t readOutGetTopLeftY();
uint8_t readOutGetBottomRightY();

#endif
