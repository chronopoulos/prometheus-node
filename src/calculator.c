#include "calculator.h"
#include "calculation.h"
#include "evalkit_constants.h"
#include "config.h"
#include "fp_atan2.h"
#include "fp_atan2_lut.h"
#include "saturation.h"
#include "adc_overflow.h"
#include "hysteresis.h"
#include "calibration.h"
#include "pru.h"
#include <math.h>
#include <stdlib.h>

/// \addtogroup calculator
/// @{

#define PI  3.14159265
#define PI2 6.28318530

// for the whole picture
int minAmplitude;
int8_t amplitudeDivider;

// for each pixel
uint16_t pixelDCS0, pixelDCS1, pixelDCS2, pixelDCS3;
int32_t arg1, arg2;
int32_t distance;
unsigned int indexMemory;
unsigned int indexCal;


static uint16_t pixelMask = 0xFFF; // epc660, epc502

uint16_t calculatorGetPixelMask() {
	return pixelMask;
}

void calculatorSetPixelMask(uint16_t bitMask) {
	pixelMask = bitMask;
}

void calculatorInit(uint8_t const divideAmplitudeBy){
	minAmplitude = pruGetMinAmplitude();
	amplitudeDivider = divideAmplitudeBy;
}

void calculatorSetArgs2DCS(uint16_t const dcs0, uint16_t const dcs1, unsigned int const index, unsigned int const indexCalibration){
	pixelDCS0 = dcs0 & pixelMask;
	pixelDCS1 = dcs1 & pixelMask;
	pixelDCS2 = adcOverflowGetMin(); // Must not be interpreted as saturation or ADC underflow.
	pixelDCS3 = adcOverflowGetMin(); // Must not be interpreted as saturation or ADC underflow.
	indexMemory = index;
	indexCal = indexCalibration;
	saturationSet2DCS(dcs0, dcs1);

	if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
		arg1 = pixelDCS1 >> 4;
		arg2 = pixelDCS0 >> 4;
	}
	else{
		arg1 = pixelDCS1;
		arg2 = pixelDCS0;
	}
	arg1 = 2048 - arg1;
	arg2 = 2048 - arg2;
}

void calculatorSetArgs4DCS(uint16_t const dcs0, uint16_t const dcs1, uint16_t const dcs2, uint16_t const dcs3, unsigned int const index, unsigned int const indexCalibration){
	pixelDCS0  = dcs0 & pixelMask;
	pixelDCS1  = dcs1 & pixelMask;
	pixelDCS2  = dcs2 & pixelMask;
	pixelDCS3  = dcs3 & pixelMask;
	saturationSet4DCS(dcs0, dcs1, dcs2, dcs3);
	indexMemory = index;
	indexCal = indexCalibration;

	arg1 = pixelDCS3 - pixelDCS1;
	arg2 = pixelDCS2 - pixelDCS0;

	if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
		arg1 = arg1 >> 4;
		arg2 = arg2 >> 4;
	}
}

int16_t calculatorGetAmpSine(){
	if (saturationCheck()){
		return SATURATION;
	}else if(adcOverflowCheck4DCS(pixelDCS0, pixelDCS1, pixelDCS2, pixelDCS3)){
		return ADC_OVERFLOW;
	}else{
		return sqrt(arg1 * arg1 + arg2 * arg2) / amplitudeDivider;
	}
}

struct TofResult calculatorGetDistAndAmpSine(){
	struct TofResult tofResult;
	tofResult.amp = calculatorGetAmpSine();
	if (saturationCheck()){
		tofResult.dist = SATURATION;
		tofResult.amp  = SATURATION;
	}else if(adcOverflowCheck4DCS(pixelDCS0, pixelDCS1, pixelDCS2, pixelDCS3)){
		tofResult.dist = ADC_OVERFLOW;
		tofResult.amp  = ADC_OVERFLOW;
	}else if(hysteresisUpdate(indexMemory, tofResult.amp)){
			/*double distance = atan2(arg1, arg2) + PI;
			distance = (distance * MAX_DIST_VALUE) / PI2;
			distance = ((int32_t)distance + MODULO_SHIFT) % MAX_DIST_VALUE;
			tofResult.dist = (int16_t) distance;*/
			tofResult.dist = (int16_t)((atan2_lut(arg1, arg2) + MODULO_SHIFT_PI) % MAX_DIST_VALUE); //atan2 from lookup table
	}else{
		tofResult.dist = LOW_AMPLITUDE;
		tofResult.amp  = LOW_AMPLITUDE;
	}

	return tofResult;
}

int16_t calculatorGetAmpPN(){
	if (saturationCheck()){
		return SATURATION;
	}else if(adcOverflowCheck4DCS(pixelDCS0, pixelDCS1, pixelDCS2, pixelDCS3)){
		return ADC_OVERFLOW;
	}else{
		return (abs(arg1) + abs(arg2)) / amplitudeDivider;
	}
}

struct TofResult calculatorGetDistAndAmpPN(){
	struct TofResult tofResult;
	tofResult.amp = (abs(arg1) + abs(arg2)) / amplitudeDivider;
	if (saturationCheck()){
		tofResult.dist = SATURATION;
		tofResult.amp  = SATURATION;
	}else if(adcOverflowCheck4DCS(pixelDCS0, pixelDCS1, pixelDCS2, pixelDCS3)){
		tofResult.dist = ADC_OVERFLOW;
		tofResult.amp  = ADC_OVERFLOW;
	}else if(hysteresisUpdate(indexMemory, tofResult.amp)){
		int32_t distance = ((double)abs(arg1) / (abs(arg1) + abs(arg2))) * MAX_DIST_VALUE + 0.5;
		tofResult.dist   = (int16_t) distance;
	}else{
		tofResult.dist = LOW_AMPLITUDE;
		tofResult.amp  = LOW_AMPLITUDE;
	}
	return tofResult;
}

/// @}
