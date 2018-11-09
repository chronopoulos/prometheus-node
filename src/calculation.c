
#define _GNU_SOURCE
#include "calculation.h"
#include "calc_def_pi_delay_sorted.h"
#include "calc_def_no_pi_delay_sorted.h"
#include "calc_mgx_pi_delay_sorted.h"
#include "calc_mgx_no_pi_delay_sorted.h"
#include "calc_pn_def_pi_delay_sorted.h"
#include "calc_pn_def_no_pi_delay_sorted.h"
#include "calc_pn_mgx_pi_delay_sorted.h"
#include "calc_pn_mgx_no_pi_delay_sorted.h"
#include "calc_bw_sorted.h"
#include "adc_overflow.h"
#include "pru.h"
#include "api.h"
#include "i2c.h"
#include "flim.h"
#include "config.h"
#include "calibration.h"
#include "saturation.h"
#include "adc_overflow.h"
#include "image_correction.h"
#include "image_processing.h"
#include "image_averaging.h"
#include "image_difference.h"
#include "temperature.h"
#include "helper.h"
#include "read_out.h"
#include "logger.h"
#include <sys/types.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/// \addtogroup calculate
/// @{

#define START  1
#define STOP   0
#define BRIGHT 1
#define DARK   0

#define BRIGHT_MGA 1
#define DARK_MGA   0
#define BRIGHT_MGB 3
#define DARK_MGB   2
#define MGA	   0
#define MGB    1

static int piDelay = 1;
static int dualMGX = 0;
static int hdr     = 0;
static int linear  = 0;
static int pn      = 0;

static int    gCounterVariation[MAX_NUM_PIX];
static double gKalmanX [MAX_NUM_PIX];
static double gKalmanP [MAX_NUM_PIX];
static double lastValid[MAX_NUM_PIX];
static double gLastVal [MAX_NUM_PIX];
static double gLastInKalman1 [MAX_NUM_PIX];

static double gK=1.0;
static double gK_max=0.1;
static double gKalman3Diff = 0.1;
static double gKalmanK = 0.1; //for simple kalman
static double gKalmanQ = 2.0; //for kalman
static double gR[MAX_NUM_PIX];
static int    gKalmanThreshold  = 500;
static int    gKalmanThreshold2 = 500;
static int 	  gNumCheck = 0;
static int 	  gThresholdCheck [MAX_NUM_PIX];
static int    gEnableKalman = 0;

static uint16_t g_nInit[MAX_NUM_PIX];
static double grayScaleOffset[MAX_NUM_PIX]; //array for grayscale DSNU
static double grayScaleGain[MAX_NUM_PIX];	//array for grayscale PRNU
static int    grayscaleBright[MAX_NUM_PIX];
static int    grayscaleDark[MAX_NUM_PIX];

static double flimOffset[2][MAX_NUM_PIX];	//array for flim mga and mgb DSNU
static double flimGain[2][MAX_NUM_PIX];		//array for flim mga and mgb PRNU
static int    flimBright[MAX_NUM_PIX];
static int    flimDark[MAX_NUM_PIX];

static int correctGrayscaleOffset = 0;
static int correctGrayscaleGain   = 0;

static int enableDRNU         = 0;
static int enableTemperature  = 0;
static int enableAmbientLight = 0;

static uint16_t gAmpGrayscale[MAX_NUM_PIX];
static int16_t  gTempDiff = 0;
static double 	gTempDiffLast = 0.0;
static double   gAmbientLightFactor    = 250;
static double   gAmbientLightFactorOrg = 250;
static int      g_imaging = 1;

static int correctFlimOffset	   = 0;
static int correctFlimGain		   = 0;
static int availableFlimCorrection = 0;

/*!
 Enabling / disabling imaging
@param status 0: disable 1: enable
@returns actual status
 */
int16_t setEnableImaging(const int status){
	g_imaging = status;
	return (uint16_t)g_imaging;
}

/*!
Does grayscale image acquisition and returns sorted grayscale data array.
@param data pointer to pointer data array.
@returns image size in pixels.
@note If grayscale correction is enabled, applies grayscale correction.
 */
int calculationBWSorted(uint16_t **data) {
	int size = calcBWSorted(data);

	if (configGetCurrentMode() == FLIM && availableFlimCorrection != 0){
		calculationFlimCorrection(data, size);
	} else if(((correctGrayscaleOffset !=0) || (correctGrayscaleGain !=0)) && (configGetCurrentMode()==GRAYSCALE)){ //if use grayscale offset or/and gain correction
		calculateGrayscaleCorrection(*data);
	}

	return size;
}

/*!
Does TOF image acquisition and returns DCS sorted data array.
@param data pointer to pointer data array.
@returns image size in pixels.
@note If ambient light flag is enabled, applies ambient light correction.
 */
int calculationDCSSorted(uint16_t **data) {
	int size = 0;

	if( configIsFLIM()== 1){
		size = calculationBWSorted(data);  //if FLIM
	} else {
		/*if(enableAmbientLight != 0){	//TODO right ambientlight correction
			int numPix = pruGetNCols() * pruGetNRowsPerHalf() * pruGetNumberOfHalves();
			temperatureGetTemperature(configGetDeviceAddress());

			for(int l=0; l< numPix; l++){	//TODO temp
				gAmpGrayscale[l] = temperatureGetGrayscalePixel(l);
			}
		}*/

		size = calculationBWSorted(data);

		/*if(calculationGetEnableAmbientLight() != 0){ TODO right ambientlight correction
			calculationAmbientDCSCorrection(data, size/pruGetNDCS());
		}*/
	}

	return size;
}

/*!
Does TOF image acquisition and returns 4xDCS + Grayscale image sorted data array.
@param data pointer to pointer data array.
@returns image size in pixels.
@note If ambient light flag is enabled, applies ambient light correction.
 */

int getDCSTOFeAndGrayscaleSorted(uint16_t **data,int nDCSTOF_log) {
	int size = 0;
	int sizeGray=0;
	op_mode_t mode = GRAYSCALE;
	uint16_t *pMemGray = NULL;

	int deviceAddress = configGetDeviceAddress();
	unsigned char *i2cData = malloc(sizeof(unsigned char));

	mode = GRAYSCALE;
	pruSetDCS(1);
	configLoad(mode, configGetDeviceAddress());

//	setCorrectGrayscaleOffset(0);						//disable gray offset correction
//	setCorrectGrayscaleGain(0);							//disable gray gain correction

	i2cData[0] = 0xc8; //switch off illumination driver
	i2c(deviceAddress, 'w', 0x90, 1, &i2cData);

	sizeGray = calculationBWSorted(data);				//get Grayscale image
	pMemGray = *data;

	i2cData[0] = 0xcc; //switch off illumination driver
	i2c(deviceAddress, 'w', 0x90, 1, &i2cData);

	free(i2cData);

	print(1,2,"size grayscale: %d \n",sizeGray);

	for(int l=0; l< sizeGray; l++){
		gAmpGrayscale[l] = pMemGray[l];					//save Grayscale image for calibration and TOF DCS attachment
		if(l==41328){									//for testing only
			print(l==41328,2,"grayscale val 14328: %d \n",gAmpGrayscale[l]);
		}
	}

	mode = THREED;
	pruSetDCS(nDCSTOF_log);
	configLoad(mode, configGetDeviceAddress());

	size = calculationBWSorted(data);					//get 4 DCS image

	/*if(calculationGetEnableAmbientLight() != 0){		//TODO: right ambient light correction, do the ambient light correction with the grayscale image
		calculationAmbientDCSCorrection(data, size/pruGetNDCS());
	}*/

	print(1,2,"size tof: %d \n",size);

	BWSortedAttachGrayscale(gAmpGrayscale,sizeGray);					//add the Grayscale image to 4xTOF DCS
	size+=sizeGray;

	print(1,2,"size tof + grayscale: %d \n",size);

	return size;
}

/*!
Does TOF image acquisition and returns sorted amplitude data array.
@param data pointer to pointer data array.
@returns image size in pixels.
 */
int calculationAmplitudeSorted(uint16_t **data) {

	if (dualMGX) {
		if (piDelay) {
			if (pn) {
				return calcPNMGXPiDelayGetDataSorted(AMP, data);
			} else {
				return calcMGXPiDelayGetDataSorted(AMP, data);
			}
		} else {
			if (pn) {
				return calcPNMGXNoPiDelayGetDataSorted(AMP, data);
			} else {
				return calcMGXNoPiDelayGetDataSorted(AMP, data);
			}
		}
	} else {
		if (piDelay) {
			if (pn) {
				return calcPNDefPiDelayGetDataSorted(AMP, data);
			} else {
				return calcDefPiDelayGetDataSorted(AMP, data);
			}
		} else {
			if (pn) {
				return calcPNDefNoPiDelayGetDataSorted(AMP, data);
			} else {
				return calcDefNoPiDelayGetDataSorted(AMP, data);
			}
		}
	}
}

/*!
Does TOF image acquisition and returns sorted distance data array.
@param data pointer to pointer data array.
@returns image size in pixels.
@note If filtering and correction is enabled, applies filtering and correction.
 */
int calculationDistanceSorted(uint16_t **data) {
	int size;
	int orgOffsetLSB = calibrationGetOffsetPhase();

	//double elapsedTime;	// time measurement
	//struct timeval tv1, tv2;
	//gettimeofday(&tv1, NULL);


	if(enableDRNU !=0){
		calibrationSetOffsetPhase(0);
	}

	if (dualMGX) {
		if (piDelay) {
			if (pn) {
				size = calcPNMGXPiDelayGetDataSorted(DIST, data);
			} else {
				size = calcMGXPiDelayGetDataSorted(DIST, data);
			}
		} else {
			if (pn) {
				size = calcPNMGXNoPiDelayGetDataSorted(DIST, data);
			} else {
				size = calcMGXNoPiDelayGetDataSorted(DIST, data);
			}
		}
	} else {
		if (piDelay) {
			if (pn) {
				size =  calcPNDefPiDelayGetDataSorted(DIST, data);
			} else {
				size = calcDefPiDelayGetDataSorted(DIST, data);
			}
		} else {
			if (pn) {
				size = calcPNDefNoPiDelayGetDataSorted(DIST, data);
			} else {
				size = calcDefNoPiDelayGetDataSorted(DIST, data);
			}
		}
	}

	imageCorrect(data);
	imageProcess(data);
	imageAverage(data);
	imageDifference(data);

	if(enableDRNU == 1){
		calibrationSetOffsetPhase(orgOffsetLSB);
		calculationCorrectDRNU(data);

	}else if(enableDRNU == 2){
		calibrationSetOffsetPhase(orgOffsetLSB);
		calculationCorrectDRNU2(data);

	}else if(enableTemperature!=0){	//only temperature correction added
		calculationTemperatureCompensation(data);
	}

	calculationAddOffset(data);

	int filterMode = configStatusEnableSquareAddDcs();

	switch(filterMode){
		case 2:
			for(int n=0; n<getNfilterLoop();n++){
				//printf("square on distance\n");
				uint16_t *pMem2 = *data;
				int width =  pruGetNCols();
				int height = pruGetNRowsPerHalf() * pruGetNumberOfHalves();
				int x, y, n;
				double dist;

				for(y= 0; y < (height-1); y++){

					//l = (y+offsetY) * pitch + offsetX;
					//k = width * y;
					for(x = 0; (x < width-1); x++, n++){
						n= width * y + x;
						if(		(pMem2[n] < LOW_AMPLITUDE) &&
								(pMem2[n+1] < LOW_AMPLITUDE) &&
								(pMem2[n+width] < LOW_AMPLITUDE) &&
								(pMem2[n+width+1] < LOW_AMPLITUDE))
						{
							dist=(pMem2[n]+pMem2[n+1]+pMem2[n+width]+pMem2[n+width+1])>>2;
							pMem2[n]=(uint16_t)(dist+0.5);
						}
					}
				}
			}
			break;
		case 3:
			for(int n=0; n<getNfilterLoop();n++){
				//printf("gaussian filter on distance\n");
				uint16_t *pMem2 = *data;
				int width =  pruGetNCols();
				int height = pruGetNRowsPerHalf() * pruGetNumberOfHalves();
				int x, y, n;
				double dist;

				for(y=0; y < (height); y++){

					//l = (y+offsetY) * pitch + offsetX;
					//k = width * y;
					for(x = 1; (x < width-1); x++, n++){
						n= width * y + x;
						if(		(pMem2[n-1] < LOW_AMPLITUDE) &&
								(pMem2[n] < LOW_AMPLITUDE) &&
								(pMem2[n+1] < LOW_AMPLITUDE))
						{
							dist=((pMem2[n-1]+((pMem2[n])<<1)+pMem2[n+1])>>2);
							pMem2[n]=(uint16_t)(dist+0.5);
						}
					}
				}

				for(y=1; y < (height-1); y++){

					//l = (y+offsetY) * pitch + offsetX;
					//k = width * y;
					for(x = 0; (x < width); x++, n++){
						n= width * y + x;
						if(		(pMem2[n-width] < LOW_AMPLITUDE) &&
								(pMem2[n] < LOW_AMPLITUDE) &&
								(pMem2[n+width] < LOW_AMPLITUDE))
						{
							dist=((pMem2[n-width]+((pMem2[n])<<1)+pMem2[n+width])>>2);
							pMem2[n]=(uint16_t)(dist+0.5);
						}
					}
				}
			}
			break;
		default:
			break;
	}

	calculationFalmanFiltering(data);

	//gettimeofday(&tv2, NULL);
	//elapsedTime = (double)(tv2.tv_sec - tv1.tv_sec) + (double)(tv2.tv_usec - tv1.tv_usec)/1000000.0;
	//printf("calculationDistanceSorted in seconds: = %fs\n", elapsedTime);


	return size;
}


/*!
Does TOF image acquisition and returns sorted distance and amplitude data array.
@param data pointer to pointer data array.
@returns image size in pixels.
@note If filtering and  correction is enabled, applies filtering and  correction
 */
int calculationDistanceAndAmplitudeSorted(uint16_t **data) {
	int size;
	int orgOffsetLSB = calibrationGetOffsetPhase();

	if(enableDRNU !=0){
		calibrationSetOffsetPhase(0);
	}

	if (dualMGX) {
		if (piDelay) {
			if (pn) {
				size = calcPNMGXPiDelayGetDataSorted(INFO, data);
			} else {
				size =  calcMGXPiDelayGetDataSorted(INFO, data);
			}
		} else {
			if (pn) {
				size =  calcPNMGXNoPiDelayGetDataSorted(INFO, data);
			} else {
				size =  calcMGXNoPiDelayGetDataSorted(INFO, data);
			}
		}
	} else {
		if (piDelay) {
			if (pn) {
				size =  calcPNDefPiDelayGetDataSorted(INFO, data);
			} else {
				size = calcDefPiDelayGetDataSorted(INFO, data);
			}
		} else {
			if (pn) {
				size = calcPNDefNoPiDelayGetDataSorted(INFO, data);
			} else {
				size = calcDefNoPiDelayGetDataSorted(INFO, data);
			}
		}
	}


	//double elapsedTime;	// time measurement
	//struct timeval tv1, tv2;
	//gettimeofday(&tv1, NULL);


	imageCorrect(data);
	imageProcess(data);
	imageAverage(data);
	imageDifference(data);

	if(enableDRNU == 1){
		calibrationSetOffsetPhase(orgOffsetLSB);
		calculationCorrectDRNU(data);

	}else if(enableDRNU == 2){
		calibrationSetOffsetPhase(orgOffsetLSB);
		calculationCorrectDRNU2(data);

	}else if(enableTemperature!=0){	//only temperature correction added
		calculationTemperatureCompensation(data);
	}

	calculationAddOffset(data);

	int filterMode = configStatusEnableSquareAddDcs();

	switch(filterMode){
		case 2:
			for(int n=0; n<getNfilterLoop();n++){
				//printf("square on distance\n");
				uint16_t *pMem2 = *data;
				int width =  pruGetNCols();
				int height = pruGetNRowsPerHalf() * pruGetNumberOfHalves();
				int x, y, n;
				double dist;

				for(y= 0; y < (height-1); y++){

					//l = (y+offsetY) * pitch + offsetX;
					//k = width * y;
					for(x = 0; (x < width-1); x++, n++){
						n= width * y + x;
						if(		(pMem2[n] < LOW_AMPLITUDE) &&
								(pMem2[n+1] < LOW_AMPLITUDE) &&
								(pMem2[n+width] < LOW_AMPLITUDE) &&
								(pMem2[n+width+1] < LOW_AMPLITUDE))
						{
							dist=(pMem2[n]+pMem2[n+1]+pMem2[n+width]+pMem2[n+width+1])>>2;
							pMem2[n]=(uint16_t)(dist+0.5);
						}
					}
				}
			}
			break;
		case 3:
			for(int n=0; n<getNfilterLoop();n++){
				//printf("gaussian filter on distance\n");
				uint16_t *pMem2 = *data;
				int width =  pruGetNCols();
				int height = pruGetNRowsPerHalf() * pruGetNumberOfHalves();
				int x, y, n;
				double dist;

				for(y=0; y < (height); y++){

					//l = (y+offsetY) * pitch + offsetX;
					//k = width * y;
					for(x = 1; (x < width-1); x++, n++){
						n= width * y + x;
						if(		(pMem2[n-1] < LOW_AMPLITUDE) &&
								(pMem2[n] < LOW_AMPLITUDE) &&
								(pMem2[n+1] < LOW_AMPLITUDE))
						{
							dist=((pMem2[n-1]+((pMem2[n])<<1)+pMem2[n+1])>>2);
							pMem2[n]=(uint16_t)(dist+0.5);
						}
					}
				}

				for(y=1; y < (height-1); y++){

					//l = (y+offsetY) * pitch + offsetX;
					//k = width * y;
					for(x = 0; (x < width); x++, n++){
						n= width * y + x;
						if(		(pMem2[n-width] < LOW_AMPLITUDE) &&
								(pMem2[n] < LOW_AMPLITUDE) &&
								(pMem2[n+width] < LOW_AMPLITUDE))
						{
							dist=((pMem2[n-width]+((pMem2[n])<<1)+pMem2[n+width])>>2);
							pMem2[n]=(uint16_t)(dist+0.5);
						}
					}
				}
			}
			break;
		default:
			break;
	}

	calculationFalmanFiltering(data);

	//gettimeofday(&tv2, NULL);
	//elapsedTime = (double)(tv2.tv_sec - tv1.tv_sec) + (double)(tv2.tv_usec - tv1.tv_usec)/1000000.0;
	//printf("calculationDistanceAndAmplitudeSorted in seconds: = %fs\n", elapsedTime);


	return size;
}

/*!
 Enables or disables the Pi delay matching.
 @param state 1 for enable, 0 for disable
 */
void calculationEnablePiDelay(const int state) {
	piDelay = state;
}

/*!
 Enables or disables the dual MGX mode.
 @param state 1 for enable, 0 for disable
 */
void calculationEnableDualMGX(const int state) {
	dualMGX = state;
}

/*!
 Enables or disables the HDR mode.
 @param state 1 for enable, 0 for disable
 */
void calculationEnableHDR(const int state) {
	hdr = state;
}

/*!
 Enables or disables the calculation with square wave demodulation.
 @param state 1 for enable, 0 for disable
 */
void calculationEnableLinear(const int state) {
	linear = state;
}

/*!
 Shows if Pi delay matching is enabled or disabled.
 @return 0 if Pi delay matching is enabled or 0 if it is disabled.
 */
int calculationIsPiDelay() {
	return piDelay;
}

/*!
 Shows if dualMGX is enabled or disabled.
 @return 0 if dualMGX is enabled or 0 if it is disabled.
 */
int calculationIsDualMGX() {
	return dualMGX;
}

/*!
 Shows if HDR is enabled or disabled.
 @return 0 if HDR is enabled or 0 if it is disabled.
 */
int calculationIsHDR() {
	return hdr;
}

/*!
 Enables or disables the calculation PN mode.
 @param state 1 for enable, 0 for disable
 */
void calculationEnablePN(const int state) {
	pn = state;
}

/*!
 Shows if PN is enabled or disabled.
 @return 0 if PN is enabled or 0 if it is disabled.
 */
int calculationIsPN() {
	return pn;
}

/*!
 Gets a grayscale picture and masks the values with 0xFFF (12bit values)
 @param data pointer to the pointer pointing at the start address, where the data will be stored at
 @returns number of pixels
 */
int calculationBW(int16_t **data) {
	int size = pruGetImage((uint16_t**)data);
	int size2 = size/2;
	int32_t *pMem32 = (int32_t*) *data;
	int16_t *pMem16 = (int16_t*) *data;

	if (configGetCurrentMode() == FLIM){
		for (int i = 0; i < size2; i++) {
			int16_t value = (pMem16[i] & 0xFFF) - 2048;
			if (value < 0){
				pMem16[i] = 0;
			} else {
				pMem16[i] = value;
			}

			value = (-(((int16_t)(pMem16[i + size / 2] & 0xFFF)) - 2048));
			if (value < 0){
				pMem16[i + size2] = 0;
			} else {
				pMem16[i + size2] = value;
			}
		}
	} else {
		for (int i = 0; i < size2; i++) {
			pMem32[i] &= 0x0FFF0FFF;
		}
	}
	return size;
}


//=====================================================================================
//
//	Grayscale DSNU and PRNU correction
//
//=====================================================================================
/*!
 Enables / disables grayscale DSNU correction
 @param enable 0- disable, 1 - enable
 */
void setCorrectGrayscaleOffset(int enable){
	correctGrayscaleOffset = enable;
}

/*!
 Enables / disables grayscale PRNU correction
 @param enable 0- disable, 1 - enable
 */
void setCorrectGrayscaleGain(int enable){
	correctGrayscaleGain = enable;
}


/*!
 Applies grayscale DSNU/PRNU correction
 @param data pointer to data pointer array
 */
void calculateGrayscaleCorrection(uint16_t* data){
	int x, y, yl, yk, k, l;
	int32_t value;
	
	int width   = pruGetNCols();
	int height  = pruGetNRowsPerHalf() * pruGetNumberOfHalves();
	int pitch   = calibrateGetWidth();
    int offsetX = readOutGetTopLeftX()-4;
    int offsetY = readOutGetTopLeftY()-6;

    print(true, 2, "grayscale offsetX= %d offsetY=%d  width= %d  height = %d\n", offsetX, offsetY, width, height);

	if(correctGrayscaleOffset !=0) {
		for(y = 0; y < height; y++) {

			yl = (y + offsetY) * pitch + offsetX;
			yk = width * y;

			for(x = 0; x < width; x++) {

				l = yl + x;
			    k = yk + x;
				value = data[k];

				if(value < LOW_AMPLITUDE) {	//if not low amplitude, or not saturated or not adc overflow
					data[k] = (uint16_t)round(value - grayScaleOffset[l]);
				}
			}
		}
	}

	if(correctGrayscaleGain !=0) {
		for(y = 0; y < height; y++) {

			yl = (y+offsetY) * pitch + offsetX;
			yk = width * y;

			for(x = 0; x < width; x++) {

				l = yl + x;
			    k = yk + x;
				value = data[k];

				if(value < LOW_AMPLITUDE) {	//if not low amplitude, or not saturated or not adc overflow
					double gainCorrGray = ((double) value - 2048) * grayScaleGain[l];
					data[k] = (uint16_t)round(gainCorrGray + 2048);
				}
			}
		}

	}
}

//================
//   prepare
//================
/*!
 Creates averaged image from X number of images
 @param numAveragingImages number of averaged images
 @param data pointer to data pointer array
 */
void calculationGrayscaleMean(int numAveragingImages, int* data){
	int l, m;
	int width = pruGetNCols();
	int height = pruGetNRowsPerHalf() * pruGetNumberOfHalves();
	int numMaxPixels = width * height;
	int tempImages[numMaxPixels];
	uint16_t *pData = NULL;

	print(true, 2, "calculationGrayscaleMean: width= %d  height= %d\n", width, height);
	memset(tempImages, 0, numMaxPixels*sizeof(int)); //clean images
	apiLoadConfig(0, configGetDeviceAddress());

	for(m=0; m < numAveragingImages; m++){		//take numMean frames for averaging

		calcBWSorted(&pData); 			//take gray scale image

		for(l=0; l< numMaxPixels; l++)
			tempImages[l] += pData[l];
	}

	for(l=0; l< numMaxPixels; l++)
		data[l] = tempImages[l] / numAveragingImages;


}
/*!
Applies DSNU / PRNU correction
 */
void calculationGrayscaleGainOffset() {
	int width = pruGetNCols();
	int height = pruGetNRowsPerHalf() * pruGetNumberOfHalves();
	int numPixels = width * height;
	double avr = 0.0;

	//calculate Offset and average
	for(int i=0; i<numPixels; i++){
		grayScaleOffset[i] = (double)grayscaleDark[i] - 2048.0;
		print(i<10, 2, "grayScaleOffset[%d]= %.4e    %d\n", i, grayScaleOffset[i], grayscaleDark[i]);
		avr +=  ((double)grayscaleBright[i] - (double)grayscaleDark[i]);
	}

	avr /= numPixels;
	printf("avr = %f ", avr);

	for(int i=0; i<numPixels; i++){
		double diff = (double)grayscaleBright[i] - (double)grayscaleDark[i];

		if(diff == 0) grayScaleGain[i] = 1;
		else          grayScaleGain[i] =  avr / diff;

		print(i<20, 2, "grayScaleGain[%d]= %.4e\n", i, grayScaleGain[i]);
	}

}

/*!
 Calibrates grayscale images
 @param mode 1 - for bright images, 0 - for dar images
 @note very important first calibrate bright image, then dark image
 */
int calculationCalibrateGrayscale(int mode) {
	int numAveragingImages = 100;

	if (mode == BRIGHT){
		printf("save bright averaged %d images\n", numAveragingImages);
		calculationGrayscaleMean(numAveragingImages, grayscaleBright);
		calculateSaveGrayscaleImage(grayscaleBright); // save image
	} else {
		printf("save dark averaged %d images and calculate offset and gain\n", numAveragingImages);
		calculateLoadGrayscaleImage(grayscaleBright);
		calculationGrayscaleMean(numAveragingImages, grayscaleDark);
		calculationGrayscaleGainOffset();
		calculateSaveGrayscaleGainOffset();
	}

	return 0;
}

//================================
//	calculate
//================================
/*!
 Saves grayscale image to "./grayscale_temp_image.txt" file
 @param data pointer to data pointer array
 @returns 0 - on succsess, - 1 if error
 */
int calculateSaveGrayscaleImage(int *data){
	FILE* file;
	int x, y, l;
	int width = pruGetNCols();
	int height = pruGetNRowsPerHalf() * pruGetNumberOfHalves();
	char* str = "./grayscale_temp_image.txt";

	file = fopen(str, "w");
	if (file == NULL){
		printf("Could not open %s file.\n", str);
		return -1;
	}

	for( l=0, y=0; y < height; y++){
		for(x=0; x<width; x++, l++){
			fprintf(file, " %d", data[l]);
		}
		fprintf(file, "\n");
	}

	fclose(file);
	return 0;
}

/*!
Loads grayscale image from "./grayscale_temp_image.txt" file
@param data pointer to data pointer array
@returns 0 - on succsess, - 1 if error
 */
int calculateLoadGrayscaleImage(int *data){
	int x, y, l;
	char    *line = NULL;
	size_t   len = 0;
	ssize_t  read;
	FILE    *file;
	int width = pruGetNCols();
	int height = pruGetNRowsPerHalf() * pruGetNumberOfHalves();
	char* str = "./grayscale_temp_image.txt";

	file = fopen(str, "r");
	if (file == NULL){
		printf("Could not open %s file.\n", str);
		return -1;
	}

	for(l=0, y=0; y < height; y++){
		if((read = getline(&line, &len, file)) != -1){
			char *pch = strtok(line," ");
			for (x=0; (pch != NULL) && (x < width); x++, l++){
				if (x > width){
					printf("File does not contain a proper grayscale offset table coefficients\n");
					return -1;
				}
				int val = helperStringToInteger(pch);
				data[l] = val;
				print(x<10, 2, "%d\t", data[l]);
				pch = strtok(NULL," ");
			}	// end while
		} else {
			return -1; //error
		}

		print(true, 2, "\n");
	}	//for y

	fclose (file);
	return 0;
}

/*!
Saves grayscale DSNU/PRNU correction "./grayscale635_gain_offset_W%03i_C%03i.txt" file
@returns 0 - on succsess, - 1 if error
@note wafer_id and chip_id is in file name is included
 */
int calculateSaveGrayscaleGainOffset(){
	FILE* file;
	int x, y, l;
	int width = pruGetNCols();
	int height = pruGetNRowsPerHalf() * pruGetNumberOfHalves();
	char fileName[64];

	if (configGetPartType() == EPC635 || configGetPartType() == EPC503){
		sprintf(fileName, "./grayscale635_gain_offset_W%03i_C%03i.txt", configGetWaferID(), configGetChipID());
	} else {
		sprintf(fileName, "./grayscale660_gain_offset_W%03i_C%03i.txt", configGetWaferID(), configGetChipID());
	}

	file = fopen(fileName, "w");
	if (file == NULL){
		printf("Could not open %s file.\n", fileName);
		return -1;
	}

	printf("%s\n", fileName);

	for( l=0, y=0; y < height; y++){
		for(x=0; x < width; x++, l++){
			fprintf(file, " %.4e", grayScaleGain[l]);
		}
		fprintf(file, "\n");
	}

	for( l=0, y=0; y < height; y++){
		for(x=0; x < width; x++, l++){
			fprintf(file, " %.4e", grayScaleOffset[l]);
		}
		fprintf(file, "\n");
	}

	fclose(file);
	return 0;
}

/*!
Loads grayscale DSNU/PRNU correction from "./grayscale635_gain_offset_W%03i_C%03i.txt" file
@returns 0 - on succsess, - 1 if error
@note wafer_id and chip_id is in file name is included
 */
int calculateLoadGrayscaleGainOffset(){
	int x, y, l;
	char*   line = NULL;
	size_t  len = 0;
	ssize_t read;
	FILE*   file;
	int  width = pruGetNCols();
	int  height = pruGetNRowsPerHalf() * pruGetNumberOfHalves();
	int  numPix = width * height;
	char fileName[64];

	memset(grayScaleOffset, 0.0, numPix * sizeof(double));
	memset(grayScaleGain,   1.0, numPix * sizeof(double));


	if (configGetPartType() == EPC635 || configGetPartType() == EPC503){
		sprintf(fileName, "./grayscale635_gain_offset_W%03i_C%03i.txt", configGetWaferID(), configGetChipID());
	} else {
		sprintf(fileName, "./grayscale660_gain_offset_W%03i_C%03i.txt", configGetWaferID(), configGetChipID());
	}

	file = fopen(fileName, "r");
	if (file == NULL){
		printf("Could not open %s file.\n", fileName);
		return -1;
	}

	printf("%s\n", fileName);
	int error_gain=0;

	//load gain
	for(l=0, y=0; y<height; y++){
		if((read = getline(&line, &len, file)) != -1){
			char *pch = strtok(line," ");
			for (x=0; (pch != NULL) && (x<width); x++, l++){
				if (x > width){
					printf("File does not contain a proper grayscale gain table coefficients\n");
					return -1;
				}
				grayScaleGain[l] = helperStringToDouble(pch);
				pch = strtok(NULL," ");
			}	// end while
		} else {
			error_gain++;
			printf("error gain: %d\n", error_gain);
		}

	}	//for y

	int error_offset = 0;

	// load offset
	for(l=0, y=0; y<height; y++){
		if((read = getline(&line, &len, file)) != -1){
			char *pch = strtok(line," ");
			for (x=0; (pch != NULL) && (x<width); x++, l++){
				if (x > width){
					printf("File does not contain a proper grayscale offset table coefficients\n");
					return -1;
				}

				grayScaleOffset[l] = helperStringToDouble(pch);
				pch = strtok(NULL," ");
			}	// end while
		} else {
			error_offset++;
			printf("error offset: %d\n", error_offset);
		}

	}	//for y

	fclose (file);
	return 0;
}

//============================================================================================
//
//	DRNU correction
//
//============================================================================================
/*!
 Check if DRNU correction is enabled
 @returns DRNU correction status 0- disbled, 1- enabled
 */
int calculationGetEnableDRNU(){
	return enableDRNU;
}

/*!
 Enable / disable DRNU correction
 @param enable DRNU status flag
 */
void calculationSetEnableDRNU(int enable){

	enableDRNU = enable;

	int deviceAddress = configGetDeviceAddress();
	unsigned char *i2cData = malloc(sizeof(unsigned char));
	i2cData[0] = 0x00;
	i2c(deviceAddress, 'w', 0x71, 1, &i2cData);
	i2c(deviceAddress, 'w', 0x72, 1, &i2cData);
	i2c(deviceAddress, 'w', 0x73, 1, &i2cData);

	if(enableDRNU !=0){ //SHO 25.08.2017 || enableTemperature !=0){
		i2cData[0] = 0x04;	// manual dll control
		i2c(deviceAddress, 'w', 0xAE, 1, &i2cData);
	} else {
		/*i2cData[0] = 0x01;	//dll bypased
		i2c(deviceAddress, 'w', 0xAE, 1, &i2cData);*/
	}

	free(i2cData);

	if(enable !=0){
		uint16_t maxDistanceCM = calibrateGetSpeedOfLightDiv2() / (10 * configGetModulationFrequency(configGetDeviceAddress()));
		//double boxLSB =    DRNU_CALIBRATION_BOX_LENGTH_CM * MAX_PHASE / maxDistanceCM;  //DRNU_CALIBRATION_BOX_LENGTH_CM = 34.5 cm
		double boxLSB = (double)calibrateGetCalibrationBoxLengthMM() * MAX_PHASE / (maxDistanceCM * 10.0);  //lsi 180302
		int value = boxLSB - calibrationGetOffsetDRNU();
		calibrationSetDefaultOffsetPhase(value);
		calibrationSetOffsetPhase(value);
	}

}

/*!
 Applies DRNU correction (faster but with low accuracy method)
 @param data pointer to data pointer array
 @returns 0
 */
int calculationCorrectDRNU(uint16_t** data){
    int i, x, y, l;
	double dist, tempDiff = 0;
    int offset  = calibrationGetOffsetDRNU();
    uint16_t maxDistanceMM = calibrateGetSpeedOfLightDiv2() / configGetModulationFrequency(configGetDeviceAddress());
    double stepLSB = calibrateGetStepCalMM() * MAX_PHASE / maxDistanceMM;
    uint16_t *pMem = *data;
    int width =  pruGetNCols();
    int height = pruGetNRowsPerHalf() * pruGetNumberOfHalves();

    if( enableTemperature !=0 ){
    	/*if(enableAmbientLight == 0){
    		tempDiff = temperatureGetTemperature(configGetDeviceAddress()) - calibrateGetTempCalib();
    	}else{
    		tempDiff = gTempDiff;
    	}*/

    	tempDiff = gTempDiff;
        //Kalman temperature filtering
        double kT=getChipTempSimpleKalmanK();
        tempDiff = tempDiff*kT + gTempDiffLast *(1-kT);
        gTempDiffLast=tempDiff;
        print(true, 3, "temperature correction k: %2.4f tempDiff %2.4f \n",kT,tempDiff);

    }

    if( enableDRNU !=0 ){

    	for(l=0, y=0; y< height; y++){
    		for(x=0; x < width; x++, l++){
    			dist = pMem[l];

    			if (dist < LOW_AMPLITUDE){	//correct if not saturated
    				dist -= offset;

    				if(dist<0)	//if phase jump
    					dist += MAX_DIST_VALUE;

    				i = (int)(round(dist / stepLSB));

    				if(i<0) i=0;
    				else if (i>=50) i=49;

    				dist = (double)pMem[l] - calibrateGetDRNULutPixel(i, y, x);

    				if( enableTemperature != 0 ){
    					dist -=  tempDiff * 3.12262;	// 0.312262 = 15.6131 / 50
    				}

    				pMem[l] = (uint16_t) round(dist);

    			}	//end if LOW_AMPLITUDE
    		}	//end width
    	}	//end height
    }	//end if enableDRNU
    return 0;
}

/*!
 Applies DRNU correction (slower but with better accuracy method)
 @param data pointer to data pointer array
 @returns 0
 @note this method GUI uses by default
 */
int calculationCorrectDRNU2(uint16_t** data){
    double dist, tempDiff = 0;
    uint16_t maxDistanceMM = calibrateGetSpeedOfLightDiv2() / configGetModulationFrequency(configGetDeviceAddress());
    double stepLSB = calibrateGetStepCalMM() * MAX_PHASE / maxDistanceMM;
    int width =  pruGetNCols();
    int height = pruGetNRowsPerHalf() * pruGetNumberOfHalves();
    uint16_t *pMem = *data;	// distance and amplitude image pointer

    if( enableTemperature !=0 ){
    	/*if(enableAmbientLight == 0){
    		tempDiff = temperatureGetTemperature(configGetDeviceAddress()) - calibrateGetTempCalib();
    	}else{
        	tempDiff = gTempDiff;
    	}*/

    	tempDiff = gTempDiff;
        //Kalman temperature filtering
        double kT=getChipTempSimpleKalmanK();
        tempDiff = tempDiff*kT + gTempDiffLast *(1-kT);
        gTempDiffLast=tempDiff;
        print(true, 3, "temperature correction k: %2.4f tempDiff %2.4f \n",kT,tempDiff);

    }


    int offset    = calibrationGetOffsetDRNU();
    int offsetX   = readOutGetTopLeftX()-4;
    int offsetY   = readOutGetTopLeftY()-6;

    int numCalPix = calibrateGetHeight() * calibrateGetWidth();
    int pitch     = calibrateGetWidth();
    int i, x, y, l, k;
    int l0, l1, index;

    double koef   = calibrateGetTempCoef(configGetPartType()) * (MAX_PHASE / maxDistanceMM);
    double s0     = 0.0;
    double m0     = 0.0;
	double m1     = 0.0;
    int numDist   = calibrateGetNumDistances();
    int lastNumDistIndex = numDist-1;
    int indexRO   = (int)((MAX_PHASE - offset) / stepLSB + 0.5) - 3;
    int flagRO    = 0;
    int breakFlag = 0;
    double stepLast = 0.0;

    if(getCalibNearRangeStatus() == 1){
		for(y= 0; y < height; y++){

			l = (y+offsetY) * pitch + offsetX;
			k = width * y;

			for(x = 0; x < width; x++, k++, l++){
				dist = pMem[k];

				if (dist < LOW_AMPLITUDE){ //if not low amplitude, not saturated and not  adc overflow

					index = (int)(((dist - offset) / stepLSB) + 0.5);
					//print((l>=(width*height/2+width/2) && l<(width*height/2+width/2+20)), 2, "precalc index: %d offset: %d stepLSB: %2.4f dist: %2.4f\n", index,offset,stepLSB,dist);

					do {
						breakFlag = 0;
						i=0;

						if (index <= 20){
							//print((l>=(width*height/2+width/2) && l<(width*height/2+width/2+20)), 2, "index= %d\n", index);
							index  = 0;
							flagRO = 1;
						} else {
							index -= 20;
						}

						l0 =  index * numCalPix + l;
						l1 = l0 + numCalPix;

						for(i= index; i<lastNumDistIndex; i++, l0 += numCalPix, l1 += numCalPix){

							m0 = calibrateGetDistImagePixel(l0);
							m1 = calibrateGetDistImagePixel(l1);

							stepLast = stepLSB;
							//if(i==0)
							//	print((l>=(width*height/2+width/2) && l<(width*height/2+width/2+20)), 2, "dist= %2.4f m0= %2.4f m1= %2.4f s0= %2.4f stepLSB= %2.4f  i= %d\n", dist, m0, m1, s0, stepLSB, i);

							if(dist >= m0 && dist < m1){
								s0 =  i * stepLSB;
								print((l>=(width*height/2+width/2) && l<(width*height/2+width/2+4)), 2, "dist before correction= %2.4f \n", dist);
								dist = (calculationInterpolate(dist, m0, s0, m1, stepLast)); //dist LSB
								print((l>=(width*height/2+width/2) && l<(width*height/2+width/2+4)), 2, "dist= %2.4f m0= %2.4f m1= %2.4f s0= %2.4f stepLSB= %2.4f  i= %d\n", dist, m0, m1, s0, stepLSB, i);
								breakFlag = 1;
								break;
							}else if(i==(lastNumDistIndex-1)){
								//print((l>=(width*height/2+width/2) && l<(width*height/2+width/2+20)), 2, "offset= %d\n", offset);
								//print((l>=(width*height/2+width/2) && l<(width*height/2+width/2+20)), 2, "numDist*stepLSB= %d\n", stepMaxdist);
								//dist = dist-(m1-offset-stepMaxdist)-offset;
								//print((l>=(width*height/2+width/2) && l<(width*height/2+width/2+20)), 2, "dist= %2.4f m0= %2.4f m1= %2.4f s0= %2.4f stepLSB= %2.4f  i= %d\n", dist, m0, m1, s0, stepLSB, i);
								/*if(dist<0)	//rollover point
									dist+=MAX_PHASE;*/
								dist = dist-(m1-numDist*stepLSB);
								if(dist<offset)
									dist+=MAX_PHASE;

								breakFlag = 1;
								break;
							}
						}//end for dist
					}while( breakFlag != 1 );


					if( enableTemperature != 0 ){
						dist -=  tempDiff * koef;	//15.6131 mm/K, attention: tempDiff is in deciDegrees
					}

					pMem[k] = (uint16_t) round(dist);

				} //end Low_AMPLITUDE
			}	//end for y
		}	//end for x
    }else{
    	for(y= 0; y < height; y++){

			l = (y+offsetY) * pitch + offsetX;
			k = width * y;

			for(x = 0; x < width; x++, k++, l++){
				dist = pMem[k];

				if (dist < LOW_AMPLITUDE){ //if not low amplitude, not saturated and not  adc overflow

					index = (int)((dist - offset) / stepLSB + 0.5);
					breakFlag = 0;
					i=0;

					if (index <= 2){
						index  = 0;
						flagRO = 1;
					} else {
						index -= 2;
					}

					do {
						if(breakFlag == 0 && i== numDist){
							index = indexRO;
							dist += MAX_PHASE;
							breakFlag = 1;
							flagRO = 0;
						}

						l0 =  index * numCalPix + l;
						l1 = l0 + numCalPix;

						for(i= index; i<numDist; i++, l0 += numCalPix, l1 += numCalPix){

							m0 = calibrateGetDistImagePixel(l0);

							if(i==lastNumDistIndex){
								m1 = calibrateGetDistImagePixel(l)+MAX_PHASE;
								stepLast = MAX_PHASE-stepLSB*i;
							}else{
								m1 = calibrateGetDistImagePixel(l1);
								stepLast = stepLSB;
							}

							if(dist >= m0 && dist < m1){
								s0 =  i * stepLSB;
								dist = (calculationInterpolate(dist, m0, s0, m1, stepLast)); //dist LSB
								//print((l>=(width*height/2+width/2) && l<(width*height/2+width/2+20)), 2, "dist= %2.4f m0= %2.4f m1= %2.4f s0= %2.4f stepLSB= %2.4f  i= %d\n", dist, m0, m1, s0, stepLSB, i);
								breakFlag = 1;
								break;

							} else if (i>=1 && flagRO==1){
								i = numDist;
								break;
							}
						}	//end for dist

					} while( breakFlag != 1 );


					if( enableTemperature != 0 ){
						dist -=  tempDiff * koef;	//15.6131 mm/K, attention: tempDiff is in deciDegrees
					}

					pMem[k] = (uint16_t) round(dist);

				} //end Low_AMPLITUDE
			}	//end for y
		}	//end for x
    }
    return 0;
}

/*!
 Linear interpolation function. y = (x-x0)*(y1-y0)/(x1-x0) + y0;
 @param x
 @param x0
 @param y0
 @param x1
 @param step size = y1 - y0
 @returns y
 */
int calculationInterpolate(double x, double x0, double y0, double x1, double step){
	double  temp;
	if( x1 == x0 ){
		temp = round(y0);
		return (int)temp;
    } else {
    	temp = round(((x-x0)*step/(x1-x0) + y0));
    	return (int)temp;
    }
}

//=====================================================================================
//
//	Temperature correction
//
//=====================================================================================
/*!
 Check if temperature correction is enabled
 @returns status
 */
int calculationGetEnableTemperature(){
	return enableTemperature;
}

/*!
 Enable / disable temperature compensation
 @param enable 0- disable, 1- enable temperature compensation
 */
void calculationSetEnableTemperature(int enable){
	enableTemperature = enable;
	int deviceAddress = configGetDeviceAddress();
	unsigned char *i2cData = malloc(sizeof(unsigned char));
	i2cData[0] = 0x00;
	i2c(deviceAddress, 'w', 0x71, 1, &i2cData);
	i2c(deviceAddress, 'w', 0x72, 1, &i2cData);
	i2c(deviceAddress, 'w', 0x73, 1, &i2cData);

	if(enableDRNU !=0){
		i2cData[0] = 0x04;	// manual dll control
		i2c(deviceAddress, 'w', 0xAE, 1, &i2cData);
	} else {
		/*i2cData[0] = 0x01;	//dll bypased
		i2c(deviceAddress, 'w', 0xAE, 1, &i2cData);*/
	}

	free(i2cData);
}

/*!
 Applies temperature compensation
 @returns 0
 */
int calculationTemperatureCompensation(uint16_t** data){

    if( enableTemperature !=0 ){
		double tempDiff=0;
		double tempVar=0.0;
		double tempDist=0.0;
		uint16_t *pMem = *data;
	    int numPix = pruGetNCols() * pruGetNRowsPerHalf() * pruGetNumberOfHalves();

		/*if(enableAmbientLight == 0){
			tempDiff = temperatureGetTemperature(configGetDeviceAddress()) - calibrateGetTempCalib();
		}else{
			tempDiff = gTempDiff;	//is already calculated (ambient compensation)
		}*/

	    tempDiff = gTempDiff;
	    double kT=getChipTempSimpleKalmanK();
	    tempDiff = tempDiff*kT + gTempDiffLast *(1-kT);
	    gTempDiffLast=tempDiff;
	    print(true, 3, "temperature correction k: %2.4f tempDiff %2.4f \n",kT,tempDiff);

		uint16_t maxDistanceMM = calibrateGetSpeedOfLightDiv2() / configGetModulationFrequency(configGetDeviceAddress());

		print(true, 3, "calculationTemperatureCompensation: calibrateGetTempCoef= %2.6f\n",calibrateGetTempCoef(configGetPartType()));

		double koef = calibrateGetTempCoef(configGetPartType()) * (MAX_PHASE / maxDistanceMM);

		for(int l=0; l < numPix; l++){
			if (pMem[l] < LOW_AMPLITUDE) { //if not low amplitude, not saturated and not  adc overflow
				tempDist=(double)pMem[l];
				tempVar=round(tempDiff * koef);
				tempDist-=tempVar;
				pMem[l]=(uint16_t)tempDist;
				//pMem[l] -= (int)(round(tempDiff * koef));	//15.6131 mm/K, attention: tempDiff is in deciDegrees
			}
		}

	}	//end enable temperature

	return 0;
}

//===============================================================================
//
//	Offset correction
//
//===============================================================================
/*!
 Adds offset to distance calculation
 @param data pointer to data pointer array
 */
void calculationAddOffset(uint16_t **data){
	int offset = 0;
	uint16_t maxDistanceCM = 0;

	if(enableDRNU!=2){
		offset = calibrationGetOffsetPhase();

	}else if( calibrationIsDefaultEnabled() == 1 ){
			maxDistanceCM = calibrateGetSpeedOfLightDiv2() / (10 * configGetModulationFrequency(configGetDeviceAddress()));
			//offset = (int)(DRNU_CALIBRATION_BOX_LENGTH_CM * MAX_PHASE / maxDistanceCM);  //DRNU_CALIBRATION_BOX_LENGTH_CM = 34.5 cm
			offset = (double)calibrateGetCalibrationBoxLengthMM() * MAX_PHASE / (maxDistanceCM * 10.0);  //lsi 180302
	}else{
			offset = calibrationGetOffsetPhase() + calibrationGetOffsetDRNU();
	}

	uint16_t val;
	uint16_t *pMem = *data;
    int numPix = pruGetNCols() * pruGetNRowsPerHalf() * pruGetNumberOfHalves();

	for(int l=0; l<numPix; l++){
		val = pMem[l];
		if (val != LOW_AMPLITUDE && val != ADC_OVERFLOW && val!= SATURATION ) { //if not low amplitude, not saturated and not  adc overflow
			pMem[l] = (uint16_t)round(((val + offset) % MAX_DIST_VALUE));
		}
	}
}


//===============================================================================
//
//	Ambient Light correction
//
//===============================================================================
/*!
 Enables / disables ambient light correction
 @param enable ambient light correction status
 */
void calculationSetEnableAmbientLight(int enable){
	enableAmbientLight = enable;
}

/*!
 Check if ambient light correction is enabled
 @returns ambient light correction status
 */
int calculationGetEnableAmbientLight(){
	return enableAmbientLight;
}

/*!
 Applies ambient light DCS correction
 @param data pointer to data pointer array
 @param pixelPerDCS number of pixels in one DCS
 */
void calculationAmbientDCSCorrection(uint16_t **data, int pixelPerDCS){
	int x, y, k, l;
	uint16_t *pMem = *data;
	uint16_t  ampGray = 0;
	uint16_t dcs0, dcs1, new_dcs0, new_dcs1;

	print(true, 2, "gAmbientLightFactor = %2.4f  pixelPerDCS= %d \n", gAmbientLightFactor, pixelPerDCS);

    int width   = pruGetNCols();
    int height  = pruGetNRowsPerHalf() * pruGetNumberOfHalves();
    int offsetX = readOutGetTopLeftX()-4;
    int offsetY = readOutGetTopLeftY()-6;
    int pitch   = calibrateGetWidth();

	for(y = 0; y < height; y++) {

		l = (y+offsetY) * pitch + offsetX;
		k = width * y;

		for(x = 0; x < width; x++, k++, l++) {

			/*if (configGetPartType() == EPC635 || configGetPartType() == EPC503){
					ampGray = round(((gAmpGrayscale[l] >> 4) - 2048) / gAmbientLightFactor);
			}else{
					ampGray = round((gAmpGrayscale[l] - 2048) / gAmbientLightFactor);
			}*/


			if(gAmpGrayscale[l] < LOW_AMPLITUDE){
				if(gAmpGrayscale[l] <= 2048)
					ampGray=0;
				else
					ampGray = round((gAmpGrayscale[l] - 2048) / gAmbientLightFactor);
			}else{
				ampGray = round(2047 / gAmbientLightFactor);
			}

			if(ampGray>2047 || ampGray<0)
				ampGray=2047;

			if((pMem[k] < LOW_AMPLITUDE) && pMem[k + pixelPerDCS] < LOW_AMPLITUDE){
				dcs0 = pMem[k];
				dcs1 = pMem[k + pixelPerDCS];

				if((dcs0-ampGray)>0){
					pMem[k] -= ampGray;
				}else{
					pMem[k]=0;
				}

				if((dcs1-ampGray)>0){
					pMem[k + pixelPerDCS] -= ampGray;
				}else{
					pMem[k + pixelPerDCS]=0;
				}

				new_dcs0 = pMem[k];
				new_dcs1 = pMem[k + pixelPerDCS];
				print(k<10, 2, "dcs0= %d  dcs1= %d  newDCS0= %d  newDCS1= %d  ambientCompensation= %d\n", dcs0, dcs1, new_dcs0, new_dcs1, ampGray);
			}
		} //end for x
	}	//end for y
}

/*!
 Applie ambient light correctiob for one pixel
 @param pixel current pixel
 @param ampGrayscale grayscale image pixel value
 @returns new pixel value
 */
int calculationAmbientDCSCorrectionPixel(uint16_t pixel, uint16_t ampGrayscale){
	int16_t ampGray = 0;
	int result=0;
	uint16_t rightMask = getConfigBitMask();

	pixel=pixel&rightMask;
	ampGrayscale=ampGrayscale&rightMask;
	saturationSet1DCS(pixel);	//check saturation of DCS
	if((saturationCheck()!=0)|| adcOverflowCheck1DCS(pixel)!=0 || adcOverflowCheck1DCS(ampGrayscale)!=0){
		result = pixel;
	}else{
		if (configGetPartType() == EPC635 || configGetPartType() == EPC503){
			ampGray = round(((ampGrayscale >> 4) - 2048) / gAmbientLightFactor);
			ampGray = (ampGray<<4) & 0xfff0;
		} else {
			ampGray = round((ampGrayscale - 2048) / gAmbientLightFactor);
		}

		result = (pixel - ampGray);
	}

	return result; //new pixel
}

/*!
 Sets ambient light factor coefficient
 @param factor ambient light factor
 @returns 0
 */
int calculationSetAmbientLightFactor(double factor){
	gAmbientLightFactor = factor;
	return 0;
}

/*!
 Sets ambient light default factor coefficient
 @param factor ambient light factor
 @returns 0
 */
void calculationSetAmbientLightFactorOrg(double factor){
	gAmbientLightFactorOrg = factor;
}

/*!
 Gets ambient light default factor coefficient
 @returns ambient light factor coefficient
 */
double calculationGetAmbientLightFactorOrg(){
	return gAmbientLightFactorOrg;
}

/*!
 Gets ambient light factor coefficient
 @returns ambient light factor coefficient
 */
double calculationGetAmbientLightFactor(){
	return gAmbientLightFactor;
}

/*!
 Sets temperature difference for DRNU calibration
 @param temperature difference in grad * 10
 */
void calculationSetDiffTemp(int tempDiff){
	gTempDiff = tempDiff;
}

//===========================================================================
//
//		Kalman filter
//
//===========================================================================
/*!
 Enable / disable kalman filtering
 @param enableKalman  0 - disable, 1 - enable
 */
void calculationEnableKalman(int enableKalman){
	gEnableKalman = enableKalman;
	memset(gCounterVariation, 0, MAX_NUM_PIX * sizeof(int));
}

/*!
 Sets Kalman diff coefficient
 @param koef
 */
void calculationSetKalmanKdiff(double koef){
	gKalman3Diff = koef;
}

/*!
 Sets Kalman gain coefficient K
 @param koef gain coefficient K
 */
void calculationSetKalmanK(double koef){
	gKalmanK = koef;
	gK_max   = koef;
}

/*!
 Gets Kalman gain coefficient K
 @returns kalman gain coefficient K
 */
double calculationGetKalmanK(){
	return gKalmanK;
}

/*!
 Sets Kalman process noise coefficient Q
 @param koef klaman noise coefficient
 */
void calculationSetKalmanQ(double koef){
	gKalmanQ = koef;
}

/*!
 Sets Kalman reset threshold
 @param indx 0 - koef for threshold2, 1 - koef for threshold1
 @param koef kalman threshold
 */
void calculationSetKalmanThreshold(int indx, int koef){
	if(indx == 1){
		gKalmanThreshold = koef;
	}else{
		gKalmanThreshold2 = koef;
	}
}

/*!
 Sets Kalman number of checks
 @param num number of checks
 */
void calculationSetKalmanNumCheck(int num){
	gNumCheck = num;
}


/*!
 Simple Kalman filter.
 @param val current value
 @param indx current value index
 @param koef filter sensitivity coefficient 0 ... 1
 @return new value
 */
double calculationSimpleKalmanFilter(double val, int indx, double koef){
	if(fabs(val - gLastVal[indx]) < gKalmanThreshold ){
		gThresholdCheck[indx] += 1;
	}else{
		gThresholdCheck[indx] = 0;
	}

	gLastVal[indx] =  val;	//save old value
	gKalmanX[indx] =  val * koef + (1-koef) * gKalmanX[indx];

	if (gThresholdCheck[indx] >= gNumCheck){
		gThresholdCheck[indx] = gNumCheck;
		if(fabs(val - lastValid[indx]) > gKalmanThreshold){
			gKalmanX[indx]  = val;
			lastValid[indx] = val;
		}else{
			lastValid[indx] = val;
		}
	}

	print(indx==4500, 2, "print test 4500 NumCheck: %d gThresholdCheck[4500]: %d  leastValid: %2.4f \n", gNumCheck,gThresholdCheck[indx],lastValid[indx]);
	return gKalmanX[indx];
}

void calculationInitKalman(){
	memset(gKalmanP, 1.0, MAX_NUM_PIX * sizeof(double));
}

/*!
 Kalman filter.
 @param val current value
 @param indx current value index
 @param q process noise covariance - filter dinamic
 @param r measurement noise covariance
 @return new value
 */
double calculationKalmanFilter(int val, int indx, double q, double r){
	double k = 0.0; 			  //kalman gain
	double p = gKalmanP[indx];	  //1.0; //estimation error covariance
	double x = gKalmanX[indx];	  //value

	p = p + q;
	k = p /(p + r);
	p = (1 - k) * p;

	double threshold = gKalmanThreshold2;
	if( gCounterVariation[indx] > 10 ){
		threshold = sqrt(r) * 6;	// 6 sigma
	}

	print(indx==4500, 2, "print test 4500 NumCheck: %d gThresholdCheck[4500]: %d \n ", gNumCheck,gThresholdCheck[indx] );

	if( gNumCheck != 0 ){	//threshold range check

		if(fabs(val-x) < threshold ){
			gThresholdCheck[indx]+=1;
		}else{
			gThresholdCheck[indx]=0;
		}

		x = x + k * (val - x);

		if (gThresholdCheck[indx] == gNumCheck ){		//threshold range check over numCheck frames
			gThresholdCheck[indx] = 0;

			if(fabs(lastValid[indx]-val) < threshold){
				lastValid[indx] = x;				//new valid value
			}else{								//reset history
				gThresholdCheck[indx] = 0;
				x = val;
				lastValid[indx] = x;				//new valid value
				gCounterVariation[indx] = -1;
				gR[indx] = 0;
			}
		}
	}else if(fabs(val-x) < threshold ){						//reset history
		print(indx==4500, 2, "threshold = %2.4f \n",threshold);
		x = x + k * (val - x);
		lastValid[indx] = x;
	}else{
		gThresholdCheck[indx] = 0;
		x = val;
		lastValid[indx] = x;				//new valid value
		gCounterVariation[indx] = -1;
		gR[indx] = 0;
		print(indx==4500, 2, "reset Kalman 2  \n");

	}

	gKalmanP[indx] = p;
	gKalmanX[indx] = x;

	return x;
}

/*!
 Applies Kalman filtering
 @param data pointer to data pointer array
 */
void calculationFalmanFiltering(uint16_t ** data){
    static int i = 0;

	uint16_t *pMem = *data;
    int width =  pruGetNCols();
    int height = pruGetNRowsPerHalf() * pruGetNumberOfHalves();
    int numPix = width * height;
    double diffKalmanIn=0.0;

    //test
    if(i==1) i=0;
    else	 i++;

    if(gEnableKalman == 1){
    	for(int l=0; l<numPix; l++)
    	{
    		//calc difference
    		if(pMem[l] < MAX_DIST_VALUE){
    			gK=gKalmanK;	//gK > 1 is a gain of the noise
    			pMem[l] = (uint16_t)round(calculationSimpleKalmanFilter(pMem[l], l, gK));
    		}
    	}
    } else if ( gEnableKalman == 2){

    	for(int l=0; l<numPix; l++)
    	{
    		if(pMem[l] < MAX_DIST_VALUE){
				gR[l] = calculationVariance(pMem[l], gCounterVariation[l], l);
				pMem[l] = (uint16_t)round(calculationKalmanFilter(pMem[l], l, gKalmanQ, gR[l]));
				gCounterVariation[l]++;
    		}
    	}
    } else if(gEnableKalman == 3){
    	for(int l=0; l<numPix; l++)
    	{
    		//calc difference
    		if(pMem[l] < MAX_DIST_VALUE){
				if(g_nInit[l]==2){
					diffKalmanIn=fabs(gLastInKalman1[l]-pMem[l]);

					if(diffKalmanIn != 0.0){
						gK = gKalman3Diff/diffKalmanIn;	//recalc K
					}else{
						gK = gK_max;	//recalc K
					}

					gLastInKalman1[l]=pMem[l];	//save old value
					if(gK >= gK_max){
						gK = gK_max;	//gK > 1 is a gain of the noise
					}
				}else{
					gK = 1.0;	// no filtering
				}
    			pMem[l] = (uint16_t)round(calculationSimpleKalmanFilter(pMem[l], l, gK));
				g_nInit[l] = 2;
				print(l==4500, 2, "set gK 4500: %2.4f \n", gK);
    		}
    	}
    }
}

/*!
 Calculates noise variance coefficient
 @param val data value
 @param n iteration number. If n = 0, reset measurement and begin from beginning
 @param indx point position in image array
 @returns variance
 */
double calculationVariance(double val, int n, int indx){
	static double x [MAX_NUM_PIX];
	static double x2[MAX_NUM_PIX];
	double variance = 0.0;

	if(n == 0) { //reset
		x [indx]  = 0;
		x2[indx]  = 0;
		variance  = 0.0;
	} else {
		x [indx] += val;
		x2[indx] += (val * val);

		if(n<2) return 0; //avoid division from 0

		variance = ( x2[indx] - ( ( x[indx] * x[indx] ) / n )  ) / ( n - 1);
	}
	return variance;
}

//=====================================================================================
//
//	FLIM DSNU and PRNU correction
//
//=====================================================================================
/*!
 Enable / disable flim DSNU correction
 @param enable 0 - disbale, 1 - enable
 */
void calculationCorrectFlimOffset(int enable){
	correctFlimOffset = enable;
}

/*!
 Enable / disable flim PRNU correction
 @param enable 0 - disbale, 1 - enable
 */
void calculationCorrectFlimGain(int enable){
	correctFlimGain = enable;
}

/*!
 Applie flim DSNU / PRNU correction
 @param pData pointer to data pointer array
 @param size image size mga + mgb in pixels
 */
void calculationFlimCorrection(uint16_t **pData, int size){
	int numPix = size/2;
	int i, x, y, k, l;
	int value;
	uint16_t *data = *pData;

	int width  = pruGetNCols();
	int height = pruGetNRowsPerHalf() * pruGetNumberOfHalves();
	int pitch  = calibrateGetWidth();

    int offsetX = readOutGetTopLeftX()-4;
    int offsetY = readOutGetTopLeftY()-6;

	if(correctFlimOffset !=0) {
		for(i=0; i<2; i++){ //mga, mgb
			for(y = 0; y < height; y++) {
				for(x = 0; x < width; x++) {
					l = (y+offsetY) * pitch + (x+offsetX);
					k = numPix * i + width * y + x;
					value = data[k];
					print((x>80 && x<83 && y>30 && y<33), 2,"flimValue= %d\n", value);

					if(value < LOW_AMPLITUDE) {	//if not low amplitude, or not saturated or not adc overflow
						data[k] = (uint16_t)round(value - flimOffset[i][l]);
					}
				}	//end x
			}	//end y
		}	//end i
	}

	if(correctFlimGain !=0) {
		for(i=0; i<2; i++){
			for(y = 0; y < height; y++) {
				for(x = 0; x < width; x++) {
					l = (y+offsetY) * pitch + (x+offsetX);
					k = numPix * i + width * y + x;
					value = data[k];

					if(value < LOW_AMPLITUDE) {	//if not low amplitude, or not saturated or not adc overflow
						data[k] = (uint16_t)round((double)value * flimGain[i][l]);
					}
				}	//end x
			}	//end y
		}	//end i
	}

}

//=======================================================================
//   Calibration
//=======================================================================
/*!
 Calibrate flim DSNU / PRNU
 @param gates 0 - mga, 1 - mgb
 */
int calculationFlimCalibration(char* gates){
	uint16_t flimRepetitions;
	uint16_t flimFlashWidth;

	if(strcmp(gates, "mga") == 0){
		printf("calibrate flim mga\n");
		calculationCalibrateFlim(BRIGHT_MGA);
		flimRepetitions = flimGetRepetitions();
		flimFlashWidth  = flimGetFlashWidth();
		printf("mga flashWidth= %d  repetitions= %d\n", flimFlashWidth, flimRepetitions);
		flimSetRepetitions(1);
		flimSetFlashWidth(0);
		calculationCalibrateFlim(DARK_MGA);
		flimSetRepetitions(flimRepetitions);
		flimSetFlashWidth(flimFlashWidth);
	}else{
		printf("calibrate flim mgb\n");
		calculationCalibrateFlim(BRIGHT_MGB);
		flimRepetitions = flimGetRepetitions();
		flimFlashWidth  = flimGetFlashWidth();
		printf("mgb flashWidth= %d  repetitions= %d\n", flimFlashWidth, flimRepetitions);
		flimSetRepetitions(1);
		flimSetFlashWidth(0);
		calculationCalibrateFlim(DARK_MGB);
		flimSetRepetitions(flimRepetitions);
		flimSetFlashWidth(flimFlashWidth);
	}

	availableFlimCorrection = 1;
	return 0;
}


/*!
 Calibration for flim DSNU / PRNU correction by using 100 images
 @param mode 0 - mga, 1 - mgb
 */
int calculationCalibrateFlim(int mode) {
	int numAveragingImages = 100;

	if (mode == BRIGHT_MGA){
		printf("save bright mga averaged %d images\n", numAveragingImages);
		calculationFlimMean(numAveragingImages, flimBright, MGA);
		calculateSaveFlimImage(flimBright); // save image
	} else if(mode == DARK_MGA) {
		printf("save dark mga averaged %d images and calculate offset and gain\n", numAveragingImages);
		calculateLoadFlimImage(flimBright);
		calculationFlimMean(numAveragingImages, flimDark, MGA);
		calculationFlimGainOffset(MGA);
		calculateSaveFlimGainOffset(MGA);
	} else if (mode == BRIGHT_MGB){
		printf("save bright mgb averaged %d images\n", numAveragingImages);
		calculationFlimMean(numAveragingImages, flimBright, MGB);
		calculateSaveFlimImage(flimBright); // save image
	} else {
		printf("save dark mgb averaged %d images and calculate offset and gain\n", numAveragingImages);
		calculateLoadFlimImage(flimBright);
		calculationFlimMean(numAveragingImages, flimDark, MGB);
		calculationFlimGainOffset(MGB);
		calculateSaveFlimGainOffset(MGB);
	}

	return 0;
}

/*!
 Creates flim averaged image from X images
 @param numAveragingImages number of images
 @param data pointer to data pointer array
 @param side  0- mga,  1- mgb
 */
int calculationFlimMean(int numAveragingImages, int* data, int side){
	int l, k, m, size=0;
	int beg = 0;
	int width = pruGetNCols();
	int height = pruGetNRowsPerHalf() * pruGetNumberOfHalves();
	int numMaxPixels = width * height;
	int tempImages[numMaxPixels];
	uint16_t *pData = NULL;

	print(true, 2, "calculationFlimMean: width= %d  height= %d\n", width, height);
	memset(tempImages, 0, numMaxPixels*sizeof(int));

	for(m=0; m < numAveragingImages; m++){	//take numMean frames for averaging

		size = calcBWSorted(&pData); 				//take gray scale image
		beg = side * size /2;

		for(k=beg, l=0; l<numMaxPixels; l++, k++)
			tempImages[l] += pData[k];

	}

	for(l=0; l<numMaxPixels; l++)
		data[l] = tempImages[l] / numAveragingImages;

	return size;
}

/*!
 Applies flim DSNU / PRNU correction
 @param side 0- mga, 1- mgb
 */
void calculationFlimGainOffset(int side) {
	int width = pruGetNCols();
	int height = pruGetNRowsPerHalf() * pruGetNumberOfHalves();
	int numPixels = width * height;
	double avr = 0.0;

	//calculate Offset and average
	for(int i=0; i<numPixels; i++){
		flimOffset[side][i] = (double)flimDark[i];
		print(i<4, 2, "flimOffset[%d][%d]= %2.4f    %d\n", side, i, flimOffset[side][i], flimDark[i]);
		avr +=  ((double)flimBright[i] - (double)flimDark[i]);
	}

	avr /= numPixels;

	for(int i=0; i<numPixels; i++){
		double diff = (double)flimBright[i] - (double)flimDark[i];

		if(diff == 0) flimGain[side][i] = 1;
		else          flimGain[side][i] =  avr / diff;

		print(i<4, 2,"flimGain[%d][%d]= %2.4f  avr= %2.4f\n", side, i, flimGain[side][i], avr);
	}

}

//=========================================================
/*!
 Saves flim image to "./flim_temp_image.txt" file
 @param data pointer to data pointer array
 @returns 0 - on succsess, -1 - error
 */
int calculateSaveFlimImage(int *data){
	FILE* file;
	int x, y, l;
	int width = pruGetNCols();
	int height = pruGetNRowsPerHalf() * pruGetNumberOfHalves();
	char* str = "./flim_temp_image.txt";

	file = fopen(str, "w");
	if (file == NULL){
		printf("Could not open %s file.\n", str);
		return -1;
	}

	for(l=0, y=0; y < height; y++){
		for(x=0; x<width; x++, l++){
			fprintf(file, " %d", data[l]);
		}
		fprintf(file, "\n");
	}

	fclose(file);
	return 0;
}

/*!
 Loads flim image from "./flim_temp_image.txt" file
 @param data pointer to data pointer array
 @returns 0 - on succsess, -1 - error
 */
int calculateLoadFlimImage(int *data){
	int x, y, l;
	char    *line = NULL;
	size_t   len = 0;
	ssize_t  read;
	FILE    *file;
	int width = pruGetNCols();
	int height = pruGetNRowsPerHalf() * pruGetNumberOfHalves();
	char* str = "./flim_temp_image.txt";

	file = fopen(str, "r");
	if (file == NULL){
		printf("Could not open %s file.\n", str);
		return -1;
	}

	for(l=0, y=0; y < height; y++){
		if((read = getline(&line, &len, file)) != -1){
			char *pch = strtok(line," ");
			for (x=0; (pch != NULL) && (x < width); x++, l++){
				if (x > width){
					printf("File does not contain a proper flim offset table coefficients\n");
					return -1;
				}
				int val = helperStringToInteger(pch);
				data[l] = val;

				print(x<10, 2, "%d\t", data[l]);

				pch = strtok(NULL," ");
			}	// end while
		} else {
			return -1; //error
		}

		print(true, 2, "\n");
	}	//for y

	fclose (file);
	return 0;
}


/*!
 Saves flim DSNE / PRNU correction file
 @param side 0- mga, 1 - mgb
 @returns 0 - on succsess, -1 - error
 */
int calculateSaveFlimGainOffset(int side){
	FILE* file;
	int  x, y, l;
	int  width = pruGetNCols();
	int  height = pruGetNRowsPerHalf() * pruGetNumberOfHalves();
	char fileName[64];

	if (configGetPartType() == EPC635 || configGetPartType() == EPC503){
		if(side == MGA) sprintf(fileName, "./epc503_mga_gain_offset_W%03i_C%03i.txt", configGetWaferID(), configGetChipID());
		else 			sprintf(fileName, "./epc503_mgb_gain_offset_W%03i_C%03i.txt", configGetWaferID(), configGetChipID());
	} else {
		if(side == MGA)	sprintf(fileName, "./epc502_mga_gain_offset_W%03i_C%03i.txt", configGetWaferID(), configGetChipID());
		else			sprintf(fileName, "./epc502_mgb_gain_offset_W%03i_C%03i.txt", configGetWaferID(), configGetChipID());
	}

	file = fopen(fileName, "w");
	if (file == NULL){
		printf("Could not open %s file.\n", fileName);
		return -1;
	}

	printf("%s\n", fileName);

	for( l=0, y=0; y < height; y++){
		for(x=0; x < width; x++, l++){
			fprintf(file, " %.4e", flimGain[side][l]);
		}
		fprintf(file, "\n");
	}

	for( l=0, y=0; y < height; y++){
		for(x=0; x < width; x++, l++){
			fprintf(file, " %.4e", flimOffset[side][l]);
		}
		fprintf(file, "\n");
	}

	fclose(file);
	return 0;
}

/*!
 Loads flim DSNU / PRNU  "./epc503_mga_gain_offset_W%03i_C%03i.txt" correction file.
 @param side 0 - mga, 1- mgb
 @returns 0 - on succsess, -1 - error
 @note file name contains wafer_id and chip_id
 */
int calculateLoadFlimGainOffset(int side){
	int x, y, l;
	char*   line = NULL;
	size_t  len = 0;
	ssize_t read;
	FILE*   file;
	int  width = pruGetNCols();
	int  height = pruGetNRowsPerHalf() * pruGetNumberOfHalves();
	int numPix = width * height;
	char fileName[64];

	for(l=0; l<numPix; l++){
		flimOffset[side][l] = 0.0;
		flimGain[side][l]   = 1.0;
	}

	if (configGetPartType() == EPC635 || configGetPartType() == EPC503){
		if(side == MGA)	sprintf(fileName, "./epc503_mga_gain_offset_W%03i_C%03i.txt", configGetWaferID(), configGetChipID());
		else			sprintf(fileName, "./epc503_mgb_gain_offset_W%03i_C%03i.txt", configGetWaferID(), configGetChipID());
	} else {
		if(side == MGA)	sprintf(fileName, "./epc502_mga_gain_offset_W%03i_C%03i.txt", configGetWaferID(), configGetChipID());
		else			sprintf(fileName, "./epc502_mgb_gain_offset_W%03i_C%03i.txt", configGetWaferID(), configGetChipID());
	}

	file = fopen(fileName, "r");
	if (file == NULL){
		printf("Could not open %s file.\n", fileName);
		return -1;
	}

	printf("%s\n", fileName);
	int error_gain=0;

	//load gain
	for(l=0, y=0; y<height; y++){
		if((read = getline(&line, &len, file)) != -1){
			char *pch = strtok(line," ");
			for (x=0; (pch != NULL) && (x<width); x++, l++){
				if (x > width){
					printf("File does not contain a proper flime gain table coefficients\n");
					return -1;
				}
				flimGain[side][l] = helperStringToDouble(pch);
				pch = strtok(NULL," ");
			}	// end while
		} else {
			error_gain++;
			printf("error gain: %d\n", error_gain);
		}

	}	//for y

	int error_offset = 0;

	// load offset
	for(l=0, y=0; y<height; y++){
		if((read = getline(&line, &len, file)) != -1){
			char *pch = strtok(line," ");
			for (x=0; (pch != NULL) && (x<width); x++, l++){
				if (x > width){
					printf("File does not contain a proper flim offset table coefficients\n");
					return -1;
				}

				flimOffset[side][l] = helperStringToDouble(pch);
				pch = strtok(NULL," ");
			}	// end while
		} else {
			error_offset++;
			printf("error offset: %d\n", error_offset);
		}

	}	//for y

	availableFlimCorrection = 1;

	fclose (file);
	return 0;
}

/*!
 Sets flim correction flag
 @param flim correction available flag
 @returns status
 */
int calculateSetFlimCorrectionAvailable(int value){
	availableFlimCorrection = value;
	return availableFlimCorrection;
}

/*!
 Check if flim correction is calibrated and available
 @returns flim correction status
 */
int calculateIsFlimCorrectionAvailable(){
	return availableFlimCorrection;
}

/// @}

