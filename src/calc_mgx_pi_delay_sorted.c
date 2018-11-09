#include "calc_mgx_pi_delay_sorted.h"
#include "evalkit_constants.h"
#include "pru.h"
#include "fp_atan2.h"
#include "calibration.h"
#include "hysteresis.h"
#include "adc_overflow.h"
#include "saturation.h"
#include "calculator.h"
#include "iterator_mgx.h"
#include <math.h>

/// \addtogroup calculate
/// @{

static uint16_t pixelData[MAX_NUM_PIX * 2];

/*!
Does MGX mode sin modulation Pi delay ( DCS0, DCS1, DCS2, DCS3)  image acquisition and returns sorted distance/amplitude data array
@param type calculation type : 0 - distance, 1-amplitude, 2 - distance and amplitude
@param data pointer to data pointer array. This arrays contains image data
@returns image size (number of pixels)
 */
int calcMGXPiDelayGetDataSorted(enum calculationType type, uint16_t **data) {
	int size = pruGetImage(data);
	uint16_t *pMem = *data;
	int nPixelPerDCS = size / 2;
	int nCols = pruGetNCols();
	uint16_t nHalves = pruGetNumberOfHalves();
	calculatorInit(2);

	iteratorMGXInit(nPixelPerDCS, nCols, pruGetNRowsPerHalf() * pruGetNumberOfHalves());
	while(iteratorMGXHasNext()){
		struct Position p = iteratorMGXNext();
		int16_t pixelDCS0 = pMem[p.indexMemory];
		int16_t pixelDCS1 = pMem[p.indexMemory + nHalves * nCols];
		int16_t pixelDCS2 = pMem[p.indexMemory + nPixelPerDCS];
		int16_t pixelDCS3 = pMem[p.indexMemory + nHalves * nCols + nPixelPerDCS];
		
		calculatorSetArgs4DCS(pixelDCS0, pixelDCS1, pixelDCS2, pixelDCS3, p.indexMemory, p.indexCalibration);
		
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
		return nPixelPerDCS;
	}
	return nPixelPerDCS / 2;
}


/// @}
