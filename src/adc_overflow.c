#include "adc_overflow.h"
#include "calculation.h"

/// \addtogroup ADC_Overflow
/// @{

static int adcOverflow = 0; //adc overflow control flag 1 - true, 0 - false

static uint16_t adcMin = 1;    // epc660, epc502
static uint16_t adcMax = 4094; // epc660, epc502

/*!
 The function returns the minimum adc value
 @returns minimum adc value
 */
uint16_t adcOverflowGetMin() {
	return adcMin;
}

/*!
 Sets the minimum adc value
 @param value minimum adc value
 */
void adcOverflowSetMin(uint16_t value) {
	adcMin = value;
}

/*!
 The function returns the maximum adc value
 @returns maximum adc value
 */
uint16_t adcOverflowGetMax() {
	return adcMax;
}

/*!
 Sets the meximum adc value
 @param value maximum adc value
 */
void adcOverflowSetMax(uint16_t value) {
	adcMax = value;
}

/*!
 Enables / disables adc overflow control.
 @param state adc overflow enable state 0- false, 1- true
 @returns current state
 */
int16_t adcOverflowEnable(int state){
	adcOverflow = state;
	return state;
}

/*!
 The function retuns current adc overflow control state
 @returns current state
 */
int adcOverflowIsEnabled(){
	return adcOverflow;
}

/*!
 Checks if dcs0 values are in range: true - if adcMin < dcs0 < adcMax.
 @param dcs0 dcs0 pixel value
 @returns state value:  0 - not in range, 1 - in range
 */
int adcOverflowCheck1DCS(uint16_t dcs0){
	if (!adcOverflow){
		return 0;
	}
	return (dcs0 < adcMin) || (dcs0 > adcMax);
}

/*!
 Checks if dcs0 and dcs1 values are in range: true - if adcMin < dcsx < adcMax.
 @param dcs0 dcs0 pixel value
 @param dcs1 dcs1 pixel value
 @returns state value:  0 - not in range, 1 - in range
 */
int adcOverflowCheck2DCS(uint16_t dcs0, uint16_t dcs1){
	if (!adcOverflow){
		return 0;
	}
	return (dcs0 < adcMin) || (dcs0 > adcMax) || (dcs1 < adcMin) || (dcs1 > adcMax);
}

/*!
 Checks if dcs0, dcs1, dcs2, dcs3 values are in range: true - if adcMin < dcsx < adcMax.
 @param dcs0 dcs0 pixel value
 @param dcs1 dcs1 pixel value
 @param dcs2 dcs2 pixel value
 @param dcs3 dcs3 pixel value
 @returns state value:  0 - not in range, 1 - in range
 */
int adcOverflowCheck4DCS(uint16_t dcs0, uint16_t dcs1, uint16_t dcs2, uint16_t dcs3){
	if (!adcOverflow){
		return 0;
	}
	return (dcs0 < adcMin) || (dcs0 > adcMax) || (dcs1 < adcMin) || (dcs1 > adcMax) || (dcs2 < adcMin)	|| (dcs2 > adcMax) || (dcs3 < adcMin) || (dcs3 > adcMax);
}

/// @}
