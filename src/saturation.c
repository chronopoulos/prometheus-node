#include "saturation.h"

/// \addtogroup saturation
/// @{

static int saturation = 0;
static uint16_t saturationBit = 0x1000; // epc660, epc502
static uint16_t saturationData;

/*!
 Sets dcs0 image pixel value for saturation control
 @param dcs0 pixel value
 */
void saturationSet1DCS(uint16_t dcs0){
	saturationData = dcs0;
}

/*!
 Sets dcs0, dcs1 pixel value for saturation control
 @param dcs0 pixel value
 @param dcs1 pixel value
 */
void saturationSet2DCS(uint16_t dcs0, uint16_t dcs1){
	saturationData = dcs0 | dcs1;
}

/*!
 Sets dcs0, dcs1, dcs2, dcs3 pixel value for saturation control
 @param dcs0 pixel value
 @param dcs1 pixel value
 */
void saturationSet4DCS(uint16_t dcs0, uint16_t dcs1, uint16_t dcs2, uint16_t dcs3){
	saturationData = dcs0 | dcs1 | dcs2 | dcs3;
}

/*!
 Enable / disable saturation control
 @param state 0 - disable, 1 - enable saturation control
 @returns saturation state
 */
int16_t saturationEnable(int state){
	saturation = state;
	return state;
}

/*!
 Check saturation
 @returns saturation flag 0  - if not saturated, 1 - if saturated
 */
int saturationCheck(){
	if (!saturation){
		return 0;
	}
	return saturationData & saturationBit;
}

/*!
 Sets saturation mask
 @param bitMask saturation bit
 */
void saturationSetSaturationBit(uint16_t bitMask) {
	saturationBit = bitMask;
}

/// @}
