#include "calc_def_pi_delay_sorted.h"
#include "pru.h"
#include "api.h"
#include "fp_atan2.h"
#include "calibration.h"
#include "config.h"
#include "hysteresis.h"
#include "adc_overflow.h"
#include "saturation.h"
#include "calculator.h"
#include "iterator_default.h"
#include "evalkit_constants.h"
#include "calculation.h"
#include "config.h"
#include "temperature.h"
#include <math.h>

#include <sys/time.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/// \addtogroup calculate
/// @{

static uint16_t pixelData[MAX_NUM_PIX * 2];
static uint16_t pMem[4 * MAX_NUM_PIX];

/*!
Does sin modulation Pi delay ( DCS0, DCS1, DCS2, DCS3)  image acquisition and returns sorted distance/amplitude data array
@param type calculation type: 0 - distance, 1-amplitude, 2 - distance and amplitude
@param data pointer to data pointer array. This arrays contains image data
@returns image size (number of pixels)
 */
int calcDefPiDelayGetDataSorted(enum calculationType type, uint16_t **data){
	int numPix = 4 * pruGetNCols() * pruGetNRowsPerHalf() * pruGetNumberOfHalves();
	int dcsAdd0=0;
	int dcsAdd1=0;
	int nPixelPerDCS = 0;
	int size=0;

	uint16_t pixelDCS0=0;
	uint16_t pixelDCS1=0;
	uint16_t pixelDCS2=0;
	uint16_t pixelDCS3=0;

	uint16_t ampGrayscale=0;

	struct TofResult tofResult;

	uint16_t* pMem1;

	/*double elapsedTime;
	struct timeval tv1, tv2;
	gettimeofday(&tv1, NULL);*/

	if (configStatusEnableAddArg()!=0){
		size = pruGetImage(data);
		pMem1 = *data;

		nPixelPerDCS = size / 4;

			for(int i=0; i<(nPixelPerDCS); i++){
				dcsAdd0=pMem1[i]+pMem1[i+2*nPixelPerDCS];	//check dcs0+2
				dcsAdd1=pMem1[i+nPixelPerDCS]+pMem1[i+3*nPixelPerDCS];	//check dcs0+2
				if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
					dcsAdd0=dcsAdd0 >> 4;
					dcsAdd1=dcsAdd1 >> 4;
				}

				if((fabs(dcsAdd0-4096) < (float)configGetDcsAdd_threshold())&&(fabs(dcsAdd1-4096) < (float)configGetDcsAdd_threshold())){
					pMem[i] = pMem1[i];
					pMem[i+nPixelPerDCS] = pMem1[i+nPixelPerDCS];
					pMem[i+2*nPixelPerDCS] = pMem1[i+2*nPixelPerDCS];
					pMem[i+3*nPixelPerDCS] = pMem1[i+3*nPixelPerDCS];
				}else{
					pMem[i] = 0x0000;
					pMem[i+nPixelPerDCS] = 0x0000;
					pMem[i+2*nPixelPerDCS] = 0x0000;
					pMem[i+3*nPixelPerDCS] = 0x0000;
				}

				if(i == 4500 && configPrint()>1){
					printf("print 4500 dcs0+2: %2.4f dcs0+2: %2.4f threshold %d \n", fabs(dcsAdd0-4096),fabs(dcsAdd1-4096),configGetDcsAdd_threshold());

				}
			}
		}else{
			size = pruGetImage(data);
			//pMem = *data;
			nPixelPerDCS = size / 4;

			/*

			for(int i=0; i<numPix; i++){
				pMem[i] = pMem1[i];
			}*/

			memcpy( (void*)pMem, (void*)(*data), numPix * sizeof(uint16_t) );	//sho 25.10.17

			/*gettimeofday(&tv2, NULL);
			elapsedTime = (double)(tv2.tv_sec - tv1.tv_sec) + (double)(tv2.tv_usec - tv1.tv_usec)/1000000.0;
			printf("calculationDistanceAndAmplitudeSorted 1 in seconds: = %fs\n", elapsedTime);*/
		}

	if( (calculationGetEnableAmbientLight() !=0) || calculationGetEnableTemperature() != 0 ){
		int tempDiff = temperatureGetTemperature(configGetDeviceAddress()) - calibrateGetTempCalib();
		calculationSetDiffTemp(tempDiff);	// set temperature difference
	}

	calculatorInit(2);
	iteratorDefaultInit(nPixelPerDCS, pruGetNCols(), pruGetNRowsPerHalf() * pruGetNumberOfHalves());

	while(iteratorDefaultHasNext()){
		struct Position p = iteratorDefaultNext();
		pixelDCS0 = pMem[p.indexMemory];
		pixelDCS1 = pMem[p.indexMemory + nPixelPerDCS];
		pixelDCS2 = pMem[p.indexMemory + 2 * nPixelPerDCS];
		pixelDCS3 = pMem[p.indexMemory + 3 * nPixelPerDCS];

		//check symmetry, if not -> pruGetImage()

		if(calculationGetEnableAmbientLight() != 0){
			ampGrayscale = temperatureGetGrayscalePixel(p.indexMemory);
			pixelDCS0 = calculationAmbientDCSCorrectionPixel(pixelDCS0, ampGrayscale);
			pixelDCS1 = calculationAmbientDCSCorrectionPixel(pixelDCS1, ampGrayscale);
		}

		calculatorSetArgs4DCS(pixelDCS0, pixelDCS1, pixelDCS2, pixelDCS3, p.indexMemory, p.indexMemory);

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

	//gettimeofday(&tv2, NULL);
	//elapsedTime = (double)(tv2.tv_sec - tv1.tv_sec) + (double)(tv2.tv_usec - tv1.tv_usec)/1000000.0;
	//printf("calculationDistanceAndAmplitudeSorted 1 in seconds: = %fs\n", elapsedTime);

	return nPixelPerDCS;
}


/// @}
