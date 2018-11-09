#include "read_out.h"
#include "i2c.h"
#include "registers.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/// \addtogroup readout
/// @{

static uint16_t tlX;
static uint16_t brX;
static uint8_t tlY;
static uint8_t brY;

/*!
 Enables or disables horizontal binning
 @param state 1 for enable, 0 for disable
 @param deviceAddress i2c address of the chip (default 0x20)
 */
void readOutEnableHorizontalBinning(const int state, const int deviceAddress) {
	unsigned char *i2cData = malloc(1 * sizeof(unsigned char));
	i2c(deviceAddress, 'r', RESOLUTION_REDUCTION, 1, &i2cData);
	if (state) {
		i2cData[0] |= 0x11;
	} else {
		i2cData[0] &= 0xEE;
	}
//	printf("binningEnableHorizontal write %i \n", resolutionReduction);
	i2c(deviceAddress, 'w', RESOLUTION_REDUCTION, 1, &i2cData);
	free(i2cData);
}

uint8_t readOutIsHorizontalBinning(const int deviceAddress){
	unsigned char *i2cData = malloc(1 * sizeof(unsigned char));
	i2c(deviceAddress, 'r', RESOLUTION_REDUCTION, 1, &i2cData);
	uint8_t enabled = (i2cData[0] & 0x10) == 0x10;
	free(i2cData);
	return enabled;
}
/*!
 Enables or disables vertical binning
 @param state 1 for enable, 0 for disable
 @param deviceAddress i2c address of the chip (default 0x20)
 */
void readOutEnableVerticalBinning(const int state, const int deviceAddress) {
	unsigned char *i2cData = malloc(1 * sizeof(unsigned char));
	i2c(deviceAddress, 'r', RESOLUTION_REDUCTION, 1, &i2cData);
	if (state) {
		i2cData[0] |= 0x20;
	} else {
		i2cData[0] &= 0xDF;
	}
//	printf("binningEnableVertical write %i \n", resolutionReduction);
	i2c(deviceAddress, 'w', RESOLUTION_REDUCTION, 1, &i2cData);
	free(i2cData);
}

uint8_t readOutIsVerticalBinning(const int deviceAddress){
	unsigned char *i2cData = malloc(1 * sizeof(unsigned char));
	i2c(deviceAddress, 'r', RESOLUTION_REDUCTION, 1, &i2cData);
	uint8_t enabled = (i2cData[0] & 0x20) == 0x20;
	free(i2cData);
	return enabled;
}


/*!
 Sets the row reduction on the chip
 @param reduction factor for reduction
 @param deviceAddress i2c address of the chip (default 0x20)
 */
void readOutSetRowReduction(const int reducedBy, const int deviceAddress) {
	unsigned char reduction = reducedBy << 2;
	unsigned char *i2cData = malloc(1 * sizeof(unsigned char));
	i2c(deviceAddress, 'r', RESOLUTION_REDUCTION, 1, &i2cData);
	i2cData[0] |= reduction; //set 1
	i2cData[0] &= (reduction | 0xF3); //set 0
	i2c(deviceAddress, 'w', RESOLUTION_REDUCTION, 1, &i2cData);
	free(i2cData);
}

/*!
 Enables or disables the dual MGX motion blur reduction on the chip
 @param state 1 for enable, 0 for disable
 @param deviceAddress i2c address of the chip (default 0x20)
 */
void readOutEnableDualMGX(const int state, const int deviceAddress) {
	unsigned char *i2cData = malloc(1 * sizeof(unsigned char));
	if (state) {
		i2c(deviceAddress, 'r', RESOLUTION_REDUCTION, 1, &i2cData);
		i2cData[0] |= 0x80;
		i2c(deviceAddress, 'w', RESOLUTION_REDUCTION, 1, &i2cData);
		i2c(deviceAddress, 'r', MT_0_HI, 1, &i2cData);
		i2cData[0] = (i2cData[0] & 0xF0) | 0x04; // MGX0: DCS0, MGX1: DCS1 (motion blur reduction)
		i2c(deviceAddress, 'w', MT_0_HI, 1, &i2cData);
		i2c(deviceAddress, 'r', MT_1_HI, 1, &i2cData);
		i2cData[0] = (i2cData[0] & 0xF0) | 0x0E; // MGX0: DCS2, MGX1: DCS3 (motion blur reduction)
		i2c(deviceAddress, 'w', MT_1_HI, 1, &i2cData);
	} else {
		i2c(deviceAddress, 'r', RESOLUTION_REDUCTION, 1, &i2cData);
		i2cData[0] &= 0x7F;
		i2c(deviceAddress, 'w', RESOLUTION_REDUCTION, 1, &i2cData);

		i2c(deviceAddress, 'r', MT_0_HI, 1, &i2cData);
		i2cData[0] &= 0xF0;  // MGX0: DCS0, MGX1: DCS0 (default = HDR)
		i2c(deviceAddress, 'w', MT_0_HI, 1, &i2cData);
		i2c(deviceAddress, 'r', MT_1_HI, 1, &i2cData);
		i2cData[0] = (i2cData[0] & 0xF0) | 0x05; // MGX0: DCS1, MGX1: DCS1 (default = HDR)
		i2c(deviceAddress, 'w', MT_1_HI, 1, &i2cData);
	}
	free(i2cData);
}

/*!
 Enables or disables the dual MGX HDR on the chip
 @param state 1 for enable, 0 for disable
 @param deviceAddress i2c address of the chip (default 0x20)
 */
void readOutEnableHDR(const int state, const int deviceAddress) {
	unsigned char *i2cData = malloc(1 * sizeof(unsigned char));
	if (state) {
		i2c(deviceAddress, 'r', RESOLUTION_REDUCTION, 1, &i2cData);
		i2cData[0] |= 0x80;
		i2c(deviceAddress, 'w', RESOLUTION_REDUCTION, 1, &i2cData);

		i2c(deviceAddress, 'r', MT_0_HI, 1, &i2cData);
		i2cData[0] = (i2cData[0] & 0xF0);
		i2c(deviceAddress, 'w', MT_0_HI, 1, &i2cData);

		i2c(deviceAddress, 'r', MT_1_HI, 1, &i2cData);
		i2cData[0] = (i2cData[0] & 0xF0) | 0x05;
		i2c(deviceAddress, 'w', MT_1_HI, 1, &i2cData);

		i2c(deviceAddress, 'r', MT_2_HI, 1, &i2cData);
		i2cData[0] = (i2cData[0] & 0xF0) | 0x0A;
		i2c(deviceAddress, 'w', MT_2_HI, 1, &i2cData);

		i2c(deviceAddress, 'r', MT_3_HI, 1, &i2cData);
		i2cData[0] = (i2cData[0] & 0xF0) | 0x0F;
		i2c(deviceAddress, 'w', MT_3_HI, 1, &i2cData);

		i2c(deviceAddress, 'r', MOD_CONTROL, 1, &i2cData);
		i2cData[0] = (i2cData[0] & 0xF7) | 0x08;
		i2c(deviceAddress, 'w', MOD_CONTROL, 1, &i2cData);

	} else {
		i2c(deviceAddress, 'r', RESOLUTION_REDUCTION, 1, &i2cData);
		i2cData[0] &= 0x7F;
		i2c(deviceAddress, 'w', RESOLUTION_REDUCTION, 1, &i2cData);

		i2c(deviceAddress, 'r', MT_0_HI, 1, &i2cData);
		i2cData[0] = (i2cData[0] & 0xF0) | 0x04;
		i2c(deviceAddress, 'w', MT_0_HI, 1, &i2cData);

		i2c(deviceAddress, 'r', MT_1_HI, 1, &i2cData);
		i2cData[0] = (i2cData[0] & 0xF0) | 0x01;
		i2c(deviceAddress, 'w', MT_1_HI, 1, &i2cData);

		i2c(deviceAddress, 'r', MT_2_HI, 1, &i2cData);
		i2cData[0] = (i2cData[0] & 0xF0) | 0x0E;
		i2c(deviceAddress, 'w', MT_2_HI, 1, &i2cData);

		i2c(deviceAddress, 'r', MT_3_HI, 1, &i2cData);
		i2cData[0] = (i2cData[0] & 0xF0) | 0x0B;
		i2c(deviceAddress, 'w', MT_3_HI, 1, &i2cData);

		i2c(deviceAddress, 'r', MOD_CONTROL, 1, &i2cData);
		i2cData[0] = (i2cData[0] & 0xF7);
		i2c(deviceAddress, 'w', MOD_CONTROL, 1, &i2cData);
	}
	free(i2cData);
}


/*!
 Sets ROI on the chip
 @param topLeftX x coordinate of the top left point (0 - 322), always less than bottomRightX, must be even
 @param bottomRightX x coordinate of the bottom right point (5 - 327), always greater than topLeftX, must be odd
 @param topLeftY y coordinate of the top left point (0 - 124), always less than bottomRightY, must be even
 @param bottomRightY y coordinate of the bottom right point (1-125), always greater than topLeftY, must be odd
 @param deviceAddress i2c address of the chip (default 0x20)
 */
void readOutSetROI(const uint16_t topLeftX, const uint16_t bottomRightX, const uint8_t topLeftY, const uint8_t bottomRightY, const int deviceAddress) {
	unsigned char *i2cData = malloc(16 * sizeof(unsigned char));
	tlX = topLeftX;
	brX = bottomRightX;
	tlY = topLeftY;
	brY = bottomRightY;
	i2cData[0] = (topLeftX >> 8) & 0xFF;  //startX
	i2cData[1] = topLeftX & 0xFF;

	i2cData[2] = (bottomRightX >> 8) & 0xFF;
	i2cData[3] = bottomRightX & 0xFF;

	i2cData[4] = topLeftY;
	i2cData[5] = bottomRightY;
	i2c(deviceAddress, 'w', 0x96, 6, &i2cData);
	free(i2cData);
}

uint16_t readOutGetTopLeftX(){
	return tlX;
}

uint16_t readOutGetBottomRightX(){
	return brX;
}

uint8_t readOutGetTopLeftY(){
	return tlY;
}

uint8_t readOutGetBottomRightY(){
	return brY;
}

/// @}
