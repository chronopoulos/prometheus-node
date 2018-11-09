#include "calc_def_no_pi_delay_sorted.h"
#include "evalkit_constants.h"
#include "pru.h"
#include "fp_atan2.h"
#include "calibration.h"
#include "hysteresis.h"
#include "adc_overflow.h"
#include "saturation.h"
#include "calculator.h"
#include "calculation.h"
#include "iterator_default.h"
#include "temperature.h"
#include <math.h>
#include <string.h>

/// \addtogroup calculate
/// @{

static uint16_t pixelData[MAX_NUM_PIX * 2];
static uint16_t pMem[2 * MAX_NUM_PIX];

/*!
Does sin modulation no Pi delay ( DCS0, DCS1)  image acquisition and returns sorted distance/amplitude data array
@param type calculation type : 0 - distance, 1-amplitude, 2 - distance and amplitude
@param data pointer to data pointer array. This arrays contains image data
@returns image size (number of pixels)
 */
int calcDefNoPiDelayGetDataSorted(enum calculationType type, uint16_t **data) {
	int size = pruGetImage(data);
	//uint16_t *pMem1 = *data;
	int numPix = 4 * pruGetNCols() * pruGetNRowsPerHalf() * pruGetNumberOfHalves();

	memcpy( (void*)pMem, (void*)(*data), numPix * sizeof(uint16_t) );		//sho 25.10.17

	if(calculationGetEnableAmbientLight() !=0 ){
		int tempDiff = temperatureGetTemperature(configGetDeviceAddress()) - calibrateGetTempCalib();
		calculationSetDiffTemp(tempDiff);
	}

	int nPixelPerDCS = size / 2;
	calculatorInit(1);
	iteratorDefaultInit(nPixelPerDCS, pruGetNCols(), pruGetNRowsPerHalf() * pruGetNumberOfHalves());
	while(iteratorDefaultHasNext()){
		struct Position p = iteratorDefaultNext();
		uint16_t pixelDCS0 = pMem[p.indexMemory];
		uint16_t pixelDCS1 = pMem[p.indexMemory + nPixelPerDCS];

		if(calculationGetEnableAmbientLight() != 0){
			uint16_t ampGrayscale = temperatureGetGrayscalePixel(p.indexMemory);
			pixelDCS0 = calculationAmbientDCSCorrectionPixel(pixelDCS0, ampGrayscale);
			pixelDCS1 = calculationAmbientDCSCorrectionPixel(pixelDCS1, ampGrayscale);
		}

		calculatorSetArgs2DCS(pixelDCS0, pixelDCS1, p.indexMemory, p.indexMemory);

		struct TofResult tofResult;
		switch(type){
		case DIST:
			pixelData[p.indexSorted] = calculatorGetDistAndAmpSine().dist;
			break;
		case AMP:
			pixelData[p.indexSorted] = calculatorGetAmpSine();
			break;
		case INFO:
			tofResult = calculatorGetDistAndAmpSine();
			pixelData[p.indexSorted] = tofResult.dist;
			pixelData[p.indexSorted + nPixelPerDCS] = tofResult.amp;
			break;
		}
	}
	*data = pixelData;
	if (type == INFO){
		return nPixelPerDCS * 2;
	}
	return nPixelPerDCS;
}

/// @}
