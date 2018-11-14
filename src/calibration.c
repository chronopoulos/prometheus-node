#define _GNU_SOURCE
#include "calibration.h"
#include "calculation.h"
#include "config.h"
#include "evalkit_constants.h"
#include "evalkit_illb.h"
#include "config.h"
#include "i2c.h"
#include "api.h"
#include "calc_bw_sorted.h"
#include "pru.h"
#include "read_out.h"
#include "temperature.h"
#include "helper.h"
#include "dll.h"
#include "registers.h"
#include "seq_prog_loader.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

/// \addtogroup calibration
/// @{

#define NUMBER_OF_MEASUREMENTS	100
#define REF_ROI_HALF_WIDTH		4
#define PI						3.14159265

static double	gDelayCal;
static double	gStepCalMM;
static int32_t	gTempCal    = 0;
static int		gOffsetDRNU = 0;

static int32_t	gTempCal_Temp[8];
static double	gStepCalMM_Temp[8];
static int		gOffsetDRNU_Temp[8];
static int      gCalibrationBoxLengthMM = 345; //mm

static int		gHeight;
static int		gWidth;
static int		gNumDist;

static int16_t gData[100];
static int16_t  gTemperatureDRNU[50];
static int16_t  gDrnuLutAverage[50];
static uint16_t nColsMax = 328;
static uint16_t nCols    = 320;
static uint16_t nRowsMax = 252;
static uint16_t nRows    = 240;
static uint16_t xCenter  = 164;
static uint16_t distImagesDRNU[8][50 * MAX_NUM_PIX];
//static int8_t 	corrIdxDLLAndDemodDelaysettings[8][4];	//toDo Code in a calib regConfigFile


static int drnuLut[50][252][328];
static int drnuLutTemp[8][50 * MAX_NUM_PIX];
static int offsetPhase = 0;
static int offsetPhaseDefault = 0;
static int offsetPhaseDefaultEnabled = 1;
static int offsetPhaseDefaultFPN = 0;
static int gDiffTemp = 10; //0.7 grad C
static int gPreheatTemp = 450; //0.7 grad C
static int gDRNUCalibrateDelay = 200000;
static int gNumAveragedFrames  = 25;
static double gEpc635_TempCoef = 1.56131;
static double gEpc660_TempCoef = 1.250;
static double gSpeedOfLightDiv2 = 150000000.0;
static int tempDistance[MAX_NUM_PIX];

int16_t calibrationSearchBadPixelsWithMin(){
	uint16_t *pMem;
	uint8_t mt_0_hi_bkp = 0;
	uint8_t mt_0_lo_bkp = 0;
	struct stat st;
	unsigned char *i2cData    = malloc(8 * sizeof(unsigned char));
	unsigned char *i2cDataBkp = malloc(4 * sizeof(unsigned char));
	int deviceAddress = configGetDeviceAddress();

	apiEnableVerticalBinning(0, deviceAddress);
	apiEnableHorizontalBinning(0, deviceAddress);
	apiSetRowReduction(0, deviceAddress);
	apiSetModulationFrequency(1, deviceAddress);
	apiEnableABS(1, deviceAddress);
	apiEnablePiDelay(1, deviceAddress);
	apiEnableDualMGX(0, deviceAddress);

	op_mode_t currentMode = configGetCurrentMode();
	if (currentMode == THREED) {
		//device was in TOF mode, will change mode to GRAYSCALE for measurement
		apiLoadConfig(0, deviceAddress);
	}

	if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
		// Switch to differential read-out (ABS disabled):
		i2c(deviceAddress, 'r', MT_0_HI, 1, &i2cData);
		mt_0_hi_bkp = i2cData[0];
		i2cData[0] = mt_0_hi_bkp & 0xCF;
		i2c(deviceAddress, 'w', MT_0_HI, 1, &i2cData);
	}

	i2c(deviceAddress, 'r', ROI_TL_X_HI, 6, &i2cData);
	const int topLeftX = (i2cData[0] << 8) + i2cData[1];
	const int bottomRightX = (i2cData[2] << 8) + i2cData[3];
	const int topLeftY = i2cData[4];
	const int bottomRightY = i2cData[5];
	if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
		apiSetROI(4, 163, 6, 65, deviceAddress);
	} else {
		apiSetROI(4, 323, 6, 125, deviceAddress);
	}

	// Set integration time to 0 (min. allowed value):
	i2c(deviceAddress, 'r', INTM_HI, 4, &i2cDataBkp);
	i2cData[0] = 0x00;
	i2cData[1] = 0x01;
	i2cData[2] = 0x00;
	i2cData[3] = 0x04;
	i2c(deviceAddress, 'w', INTM_HI, 4, &i2cData);

	// Force modulation gates off, i.e. charge transfer will be blocked.
	i2c(deviceAddress, 'r', MT_0_LO, 1, &i2cData);
	mt_0_lo_bkp = i2cData[0];
	i2cData[0] = mt_0_lo_bkp | 0x03;
	i2c(deviceAddress, 'w', MT_0_LO, 1, &i2cData);

	calcBWSorted(&pMem, 0); // frame acquisition

	char path[64];
	sprintf(path, "./calibration/PT%02i_W%03i_C%03i_bad_pixels.txt", configGetPartType(), configGetWaferID(), configGetChipID());
	FILE *fileBadPixels = fopen(path, "wb");
	if (fileBadPixels == NULL){
		return -1;
	}
	
	int16_t count = 0;
	int x, y;
	for (y = 0; y < nRows; y++){
		for (x = 0; x < nCols; x++){
			if (pMem[y*nCols+x] > 2096 || pMem[y*nCols+x] < 2000){
				fprintf(fileBadPixels,"%i %i\n",x+4,y+6);
				++count;
			}
		}
	}
	fclose(fileBadPixels);

	// Restore modulation gate forcing:
	i2cData[0] = mt_0_lo_bkp;
	i2c(deviceAddress, 'w', MT_0_LO, 1, &i2cData);

	// Restore integration time settings:
	i2c(deviceAddress, 'w', INTM_HI, 4, &i2cDataBkp);
	if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
		// Restore readout settings:
		i2cData[0] = mt_0_hi_bkp;
		i2c(deviceAddress, 'w', MT_0_HI, 1, &i2cData);
	}

	if (currentMode == THREED) {
		//device was in THREED mode before measuring, will change mode to THREED
		apiLoadConfig(1, deviceAddress);
	}
	apiSetROI(topLeftX, bottomRightX, topLeftY, bottomRightY, deviceAddress);
	free(i2cData);
	free(i2cDataBkp);

	if (stat(path, &st) == 0) {
		if((int)st.st_size == 0 && count != 0) {
			remove(path);
			return -1;
		}
	} else {
		remove(path);
		return -1;
	}

	return count;
}

uint16_t calibrationGetBadPixels(int16_t* badPixels){
	FILE *fBadPixels;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;
	char path[64];
	sprintf(path, "./calibration/PT%02i_W%03i_C%03i_bad_pixels.txt", configGetPartType(), configGetWaferID(), configGetChipID());

	fBadPixels = fopen(path, "r");
	if (fBadPixels == NULL){
		printf("Could not open %s file.\n", path);
		return -1;
	}
	uint16_t i = 0;
	while((read = getline(&line, &len, fBadPixels)) != -1){
		char *pch = strtok(line," ");
		int n = 0;
		while (pch != NULL){
			n++;
			if (n > 2){
				printf("File does not contain a proper calibration.\n");
				return -1;
			}
			int16_t val = helperStringToInteger(pch);
			badPixels[i] = val;
			i++;
			pch = strtok(NULL," ");
		}
	}
	fclose(fBadPixels);
	if (line){
		free(line);
	}
	return i;
}


/*!
 Sets the offset.
 @param offset as phase
 @returns offset as phase
 */
int calibrationSetOffsetCM(const int value, const int deviceAddress) {
	offsetPhase = MAX_PHASE / (gSpeedOfLightDiv2 / configGetModulationFrequency(deviceAddress) / 10) * value;
	return offsetPhase;
}

void calibrationSetOffsetPhase(int value){
	offsetPhase = value;
}

void calibrationSetDefaultOffsetPhase(int value){
	offsetPhaseDefault = value;
}

/*!
 Getter function for the Offset (is used for the calculation on the BeagleBone)
 @returns offset as phase
 */
int calibrationGetOffsetPhase() {
	return offsetPhase;
}

/*!
 Getter function for the default offset in cm
 @returns offset in cm
 */
int calibrationGetOffsetDefaultCM(const int deviceAddress) {
	int temp=0;
	temp=offsetPhaseDefault / MAX_PHASE * gSpeedOfLightDiv2 / configGetModulationFrequency(deviceAddress) / 10;
	printf("offset CM: %d",temp);
	return temp;
}

/*!
 Enables or disables the default offset.
 @param state if 0 the Offset set over calibrationSetOffset will be used else the offset from the calibration
 @returns state
 */
int calibrationEnableDefaultOffset(int state) {
	if (state == 0) {
		offsetPhaseDefaultEnabled = 0;
	} else {
		offsetPhaseDefaultEnabled = 1;
		offsetPhase = offsetPhaseDefault;
	}
	return state;
}

int calibrationIsDefaultEnabled(){
	return offsetPhaseDefaultEnabled;
}

void calibrationSetNumberOfColumnsMax(const uint16_t value) {
	nColsMax = value;
	nCols    = nColsMax - 8;
	xCenter  = nColsMax / 2;
}

void calibrationSetNumberOfRowsMax(const uint16_t value) {
	nRowsMax = value;
	nRows    = nRowsMax - 12;
}

//=====================================================================================
//
//	calibrate DRNU correction
//
//=====================================================================================
int calibrationGetOffsetDRNU(){
	return gOffsetDRNU;
}

int calibrationDRNU(){
	int deviceAddress = configGetDeviceAddress();
	unsigned char *i2cData = malloc(2 * sizeof(unsigned char));
	op_mode_t currentMode = configGetCurrentMode();
	if (currentMode == GRAYSCALE) { //device was in GRAYSCALE mode, will change mode to TOF for measurement
		apiLoadConfig(1, deviceAddress);
	}

	if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
		apiSetROI(4, 163, 6, 65, deviceAddress);
	} else {
		apiSetROI(4, 323, 6, 125, deviceAddress);
	}

	//prepare for calibration
	int16_t minAmplitude = apiGetMinAmplitude();
	apiSetMinAmplitude(0);
	int offsetPhase = calibrationGetOffsetPhase();
	calibrationSetOffsetPhase(0);
	int ambientLight =  calculationGetEnableAmbientLight();
	calculationSetEnableAmbientLight(0);
	int  enbleDRNUsave = calculationGetEnableDRNU();
	calculationSetEnableDRNU(0);
	int  enableTemperature = calculationGetEnableTemperature();
	calculationSetEnableTemperature(0);

	//set dll manual control
	i2cData[0] = 0x04;
	i2c(deviceAddress, 'w', 0xAE, 1, &i2cData);
	i2cData[0] = 0x00;
	i2c(deviceAddress, 'w', 0x71, 1, &i2cData);
	i2c(deviceAddress, 'w', 0x72, 1, &i2cData);

	//get calibration delay
	i2c(deviceAddress, 'r', 0xE9, 1, &i2cData);
	if(i2cData[0]!=0xff){
		gDelayCal=(double)i2cData[0];
		gDelayCal=((gDelayCal-128.0)*0.003+0.1333); //tdelay = (CalReg - 128) * tcal,step + tdelay,ref
	}else if(getCalibNearRangeStatus()==1){
		gDelayCal=0.1333;
	}else{
		gDelayCal=1.8;	//default
	}
	printf("gDelayCal: %3.2f \n",gDelayCal);

	int tempDiff = 1000;
	int count = 0;
	int tempThresholdSlope=0;

	calibrationRunPreheat(gPreheatTemp);

	if(getCalibNearRangeStatus()==1){
		while( abs(tempDiff) > gDiffTemp){	// repeat measurement till temperature difference gDiffTem decidegrees
			gTempCal = 0;
			uint16_t j=0;
			//calibrateSetDRNUDiffTemp(10);//calibrateSetDRNUDiffTemp(50);
			//calibrateSetDRNUAverage(100);//calibrateSetDRNUAverage(100);
			//i2cData[0] = 0x03;
			//i2c(deviceAddress, 'w', 0x73, 1, &i2cData);		//fine dll step

			for(int i=0;  i < gNumDist; i++ ){
				j=i*16;
				i2cData[0] = (uint8_t)(j>>8);
				//printf("j= %d\n", j);
				i2cData[1] = (uint8_t)((j & 0x00ff));
				//printf("j= %d\n", j);
				//printf("i2cData[0]= %d\n", i2cData[0]);
				//printf("i2cData[1]= %d\n", i2cData[1]);
				//printf("avargedFrames[1]= %d\n", gNumAveragedFrames);
				i2c(deviceAddress, 'w', 0x71, 2, &i2cData);		//fine dll step

				calibrationCreateDRNU_LUT(i, gNumAveragedFrames);
				//printf("gTemperatureDRNU[i]= %d\n", gTemperatureDRNU[i]);
				gTempCal += gTemperatureDRNU[i];
				tempThresholdSlope=gDiffTemp;
				if( abs(gTemperatureDRNU[i] - gTemperatureDRNU[0]) > tempThresholdSlope){
					i=-1;
					gTempCal = 0;
				}
			}

			gTempCal = (int)(round((double)gTempCal/gNumDist));

			double maxDistanceCM = gSpeedOfLightDiv2 / ( 10 * configGetModulationFrequency(configGetDeviceAddress()));
			double koef = 15.0 * MAX_PHASE / maxDistanceCM;
			double stepLSB = koef * ((TEMPERATURE_SLOPE * (double)(gTempCal-TEMPERATURE_CALIBRATION_DELAY)/10.0) + gDelayCal);
			//10mhz = 80lsb step size in LSB
			gStepCalMM = (stepLSB * gSpeedOfLightDiv2 / configGetModulationFrequency(configGetDeviceAddress()) ) / MAX_PHASE;
			tempDiff = gTemperatureDRNU[gNumDist-1] - gTemperatureDRNU[0];
			//printf("stepsize= %f\n", stepLSB);
			printf("tempDiff= %d  gTempCal= %d  count= %d gNumDist= %d \n", tempDiff, gTempCal, count,gNumDist);
			count++;

			calibrationSaveDistanceImages();
		}
	}else{
		while( abs(tempDiff) > gDiffTemp){	// repeat measurement till temperature difference gDiffTem decidegrees
			gTempCal = 0;

			for(int i=0;  i < gNumDist; i++ ){
				i2cData[0] = i;
				i2c(deviceAddress, 'w', 0x73, 1, &i2cData);
				calibrationCreateDRNU_LUT(i, gNumAveragedFrames);
				printf("gTemperatureDRNU[i]= %d\n", gTemperatureDRNU[i]);
				gTempCal += gTemperatureDRNU[i];

				tempThresholdSlope=gDiffTemp;
				if( abs(gTemperatureDRNU[i] - gTemperatureDRNU[0]) > tempThresholdSlope){
					i=-1;
					gTempCal = 0;
				}
			}

			gTempCal = (int)(round((double)gTempCal/gNumDist));

			double maxDistanceCM = gSpeedOfLightDiv2 / ( 10 * configGetModulationFrequency(configGetDeviceAddress()));
			double koef = 15.0 * MAX_PHASE / maxDistanceCM;
			double stepLSB = koef * ((TEMPERATURE_SLOPE * (double)(gTempCal-TEMPERATURE_CALIBRATION_DELAY)/10.0) + gDelayCal);	//step size in LSB
			gStepCalMM = (stepLSB * gSpeedOfLightDiv2 / configGetModulationFrequency(configGetDeviceAddress()) ) / MAX_PHASE;

			tempDiff = gTemperatureDRNU[gNumDist-1] - gTemperatureDRNU[0];
			printf("tempDiff= %d  gTempCal= %d  count= %d\n", tempDiff, gTempCal, count);
			count++;

			calibrationSaveDistanceImages();

		}
	}

	calibrationSaveTemperatureDRNU();

	calibrationDRNU_FromFile((int)(gStepCalMM * 10.0));

	i2cData[0] = 0x01;
	i2c(deviceAddress, 'w', 0xAE, 1, &i2cData);
	i2cData[0] = 0x00;
	i2c(deviceAddress, 'w', 0x71, 1, &i2cData);
	i2c(deviceAddress, 'w', 0x72, 1, &i2cData);
	i2c(deviceAddress, 'w', 0x73, 1, &i2cData);

	free(i2cData);

	if (currentMode == GRAYSCALE) { //device was in GRAYSCALE mode, will change mode to TOF for measurement
		apiLoadConfig(0, deviceAddress);
	}

	//restore original parameters
	apiSetMinAmplitude(minAmplitude);
	calibrationSetOffsetPhase(offsetPhase);
	calculationGetEnableAmbientLight(ambientLight);
	calculationSetEnableDRNU(enbleDRNUsave);
	calculationGetEnableTemperature(enableTemperature);

	return 0;
}

void calibrationCreateDRNU_LUT(int indx, int numAveragedFrames){
	int i, x, y, j, l, val;
	int topX,topY,bottomX,bottomY;
	uint16_t  *pMem   = NULL;
	printf("calibrationCreateDRNU_LUT: %d  numAvrFrames= %d\n", indx, numAveragedFrames);
	int numPix = gWidth * gHeight;
	int temperature = 0;
	int32_t avr[numPix];

	memset(avr, 0, numPix * sizeof(int));

	for(j=0; j<numAveragedFrames; j++){
		usleep(gDRNUCalibrateDelay);	//200 mS pause

		calculationDistanceSorted(&pMem, 0);

		temperature += temperatureGetTemperature(configGetDeviceAddress());

		for(l=0; l < numPix; l++){
			val = pMem[l];
			if(getCalibNearRangeStatus()!=1){
				if ( val < tempDistance[l] && indx > 0 ){
					val += MAX_PHASE;
				}
			}else{}

			avr[l] += val;
		}
	}

	temperature = (int)(round((double)temperature/numAveragedFrames));
	printf("mean temp: %d \n ", temperature);

	int average = 0;

	int k = 0;
	k=configGetModFreqMode();


	/*if(configGetModFreqMode() == MOD_FREQ_10000KHZ){
		k = 1;
	}*/

	i =  indx * numPix;

	printf("k: %d   indx: %d i: %d \n ", k,indx,i);

	for(l=0; l < numPix; l++, i++){
		val = avr[l] / numAveragedFrames;
		distImagesDRNU[k][i] = val;
		tempDistance[l]      = val;

		if(indx==0){
			average += val;
		}

		pMem[l] = val;
	}

	if(indx==0){
		//near range offset calculation sho
		if((configGetPartType() == EPC635) || (configGetPartType() == EPC503)){ // offsetRoi Calc
			topX=0;
			bottomX=gWidth;
			topY=0;
			bottomY=gHeight;
		}else{	//660 ROI DRNU offset calc
			/*int topX=50;
			int topY=50;
			int bottomX=270;
			int bottomY=190;*/
			topX=0;
			bottomX=gWidth;
			topY=0;
			bottomY=gHeight;
		}

		int counter = 0;

		for(y=0; y<gHeight; y++){
				for(x=0; x<gWidth; x++){
					if(x>topX && x<bottomX && y>topY && y<bottomY){
						l= gWidth * y + x;
						gOffsetDRNU += distImagesDRNU[k][l];
						counter++;
					}
			}
		}
		gOffsetDRNU=(int)(round(gOffsetDRNU/counter));
	}/*else if(indx==0){
		gOffsetDRNU = average /numPix;
	}*/
	//offset calculation sho

	gTemperatureDRNU[indx] = temperature;

	double maxDistanceCM = gSpeedOfLightDiv2 / ( 10 * configGetModulationFrequency(configGetDeviceAddress()));
	double koef = 15.0 * MAX_PHASE / maxDistanceCM;
	double stepLSB = koef * ((TEMPERATURE_SLOPE * (temperature-TEMPERATURE_CALIBRATION_DELAY) / 10.0) + gDelayCal);	//step size in LSB koef was 30

	for(l=0, y=0; y < gHeight; y++){
		for(x=0; x < gWidth; x++, l++){
			int value = pMem[l] - gOffsetDRNU;
			drnuLut[indx][y][x] = (int)(round(value - indx * stepLSB));	//1.0372  1.025
			print((x>=140 && x<144 && y>=120  && y<124), 2, "value= %d  pMem[%d]= %d  drnuLut[%d]= %d  offset= %d stepLSB= %2.2f  temp= %d\n", value, l, pMem[l], indx, drnuLut[indx][y][x], gOffsetDRNU, stepLSB, temperature);
		}
		print(y<1, 2, "\n");
	}

}





int calibrationDRNU_FromFile(int step){
	int i, x, y, l;
	int topX,topY,bottomX,bottomY;

	double stepLSB;
	gStepCalMM = (double)step / 10.0;

	calibrationLoadTemperatureDRNU();	// calc new gTempCal

	uint16_t maxDistanceMM = gSpeedOfLightDiv2 / configGetModulationFrequency(configGetDeviceAddress());
	stepLSB = (MAX_PHASE * gStepCalMM)  / maxDistanceMM;

	//int numPix = gHeight * gWidth; //TODO remove
	gOffsetDRNU = 0;

	int freq = 0;

	freq = configGetModFreqMode();


	/*for(l=0; l<numPix; l++){
		gOffsetDRNU += distImagesDRNU[freq][l];
	}*/



	if((configGetPartType() == EPC635) || (configGetPartType() == EPC503)){ // offsetRoi Calc
		topX=0;
		bottomX=gWidth;
		topY=0;
		bottomY=gHeight;
	}else{	//660 ROI DRNU offset calc
		/*int topX=50;
		int topY=50;
		int bottomX=270;
		int bottomY=190;*/
		topX=0;
		bottomX=gWidth;
		topY=0;
		bottomY=gHeight;
	}

    int counter = 0;

    //if(getCalibNearRangeStatus()==1){
	for(y=0; y<gHeight; y++){
			for(x=0; x<gWidth; x++){
					if(x>=topX && x<bottomX && y>topY && y<bottomY){
							l= gWidth * y + x;
							gOffsetDRNU += distImagesDRNU[freq][l];
							counter++;
					}
			}
	}
    /*}else{
		for(y=0; y<gHeight; y++){
			for(x=0; x<gWidth; x++){
				l= gWidth * y + x;
				gOffsetDRNU += distImagesDRNU[freq][l];
				counter++;
			}
		}
	}*/
    gOffsetDRNU = (int)(round(gOffsetDRNU/counter));

	if(configPrint() > 1) printf("gStepCalMM= %2.4f, stepLSB= %2.2f offsetDRNU= %d\n", gStepCalMM, stepLSB, gOffsetDRNU);

	for(i=0; i<gNumDist; i++){

		int M = i * gHeight * gWidth;

		for(l=0, y=0; y < gHeight; y++){
			for(x=0; x < gWidth; x++, l++){
				int value = distImagesDRNU[freq][M + l] - gOffsetDRNU;
				drnuLut[i][y][x] = (int)(round(value - i * stepLSB));	//1.0372  1.025
				print(( x>=140 && x<144 && y>=120  && y<124), 2, "value= %d  distImagesDRNU[%d]= %d  drnuLut[%d]= %d  offset= %d stepLSB= %2.2f  \n", value, (M+l), distImagesDRNU[freq][M + l], i, drnuLut[i][y][x], gOffsetDRNU, stepLSB);
			}

			print(y<1, 1, "\n");
		}
	}

	calibrationSaveDRNU_LUT();

	int32_t drnuLutAverage[50];

	for(int k=0; k < gNumDist; k++){

		drnuLutAverage[k] = 0;
		counter = 0;

		for(int y=topY; y<bottomY; y++){
			for(int x = topX; x<bottomX; x++){
				drnuLutAverage[k] += drnuLut[k][y][x];
				counter++;
			}
		}

		gDrnuLutAverage[k] =   (int16_t)(round(drnuLutAverage[k]/counter));
		printf("drnu_avr= %d\n", gDrnuLutAverage[k]);
	}

	return 0;
}

void configAddEndFileName(char* name, char* str){
	uint16_t freq = configGetModulationFrequency(configGetDeviceAddress());
	if(getCalibNearRangeStatus()==1){
		if(freq >= 36000){
			strcat(name, "_Calfreq7.");
		} else if( freq >= 20000){
			if(configIsABS(configGetDeviceAddress())==1){
				strcat(name, "_Calfreq6.");
			}else{
				strcat(name, "_Calfreq6_woABS.");
			}
		}
	}else{
		if(freq >= 20000){
			strcat(name, "_20.");
		} else if( freq >= 10000){
			strcat(name, "_10.");
		} else {
			strcat(name, "_5.");
		}
	}
	strcat(name, str);
}

int calibrationSaveDRNU_LUT(){
	int x, y, k;
	char fileName[64];

	sprintf(fileName, "epc660_drnu_W%03i_C%03i", configGetWaferID(), configGetChipID());
	if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
		sprintf(fileName, "epc635_drnu_W%03i_C%03i", configGetWaferID(), configGetChipID());
	}

	configAddEndFileName(fileName, "csv");

	FILE *file = fopen(fileName, "w");
	if (file == NULL){
		return -1;
	}

	printf("%s\n", fileName);

	fprintf(file,"%d\n", gOffsetDRNU);
	fprintf(file,"%d\n", gTempCal);
	fprintf(file,"%2.4f\n", gStepCalMM);

	for (k=0; k < gNumDist; k++){
		for (y = 0; y < gHeight; y++){
			for (x = 0; x < gWidth-1; x++){
				fprintf(file,"%d ", drnuLut[k][y][x]);
			}
			fprintf(file,"%d\n", drnuLut[k][y][x]);
		}
	}

	fclose(file);

	if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
		sprintf(fileName, "epc635_drnu_W%03i_C%03i_test_xy", configGetWaferID(), configGetChipID());
		configAddEndFileName(fileName, "csv");
		calibrationSaveDRNU_LUT_TestXY(fileName);

		sprintf(fileName, "epc635_drnu_W%03i_C%03i_test_yx", configGetWaferID(), configGetChipID());
		configAddEndFileName(fileName, "csv");
		calibrationSaveDRNU_LUT_TestYX(fileName);

	} else {
		sprintf(fileName, "epc660_drnu_W%03i_C%03i_test_xy", configGetWaferID(), configGetChipID());
		configAddEndFileName(fileName, "csv");
		calibrationSaveDRNU_LUT_TestXY(fileName);

		sprintf(fileName, "epc660_drnu_W%03i_C%03i_test_yx", configGetWaferID(), configGetChipID());
		configAddEndFileName(fileName, "csv");
		calibrationSaveDRNU_LUT_TestYX(fileName);
	}

	return 0;
}



int calibrationSaveTemperatureDRNU(){
	char fileName[64];

	sprintf(fileName, "epc660_temperature_drnu_W%03i_C%03i", configGetWaferID(), configGetChipID());
	if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
		sprintf(fileName, "epc635_temperature_drnu_W%03i_C%03i", configGetWaferID(), configGetChipID());
	}

	configAddEndFileName(fileName, "csv");
	printf("%s\n", fileName);

	FILE *file = fopen(fileName, "w");
	if (file == NULL){
		return -1;
	}

	for(int k=0; k < gNumDist; k++){
		fprintf(file,"%d\n", gTemperatureDRNU[k]);
	}

	fclose(file);
	return 0;
}

int calibrationRunPreheat(int preheatTemp){
	int temperature=0;
	int tempAverage=0;
	int counter = 0;
	int countRepeated=0;

	unsigned char *i2cCalibReg = malloc(1* sizeof(unsigned char));
	int deviceAddress = configGetDeviceAddress();

	illuminationDisable();
	while((tempAverage < preheatTemp) && (countRepeated < 60)){
		counter=0;
		temperature=0;

		//run in video without grap images

		i2cCalibReg[0]=0x03;
		i2c(deviceAddress, 'w', SHUTTER_CONTROL, 1, &i2cCalibReg);
		sleep(1);
		i2cCalibReg[0]=0x00;
		i2c(deviceAddress, 'w', SHUTTER_CONTROL, 1, &i2cCalibReg);
		//run in video without grap images

		for(int j=0; j<10; j++){
			temperature += temperatureGetTemperature(configGetDeviceAddress());
			counter++;
		}
		printf("counter: % d \n",counter);
		tempAverage=(int)(round((double)temperature/counter));
		printf("av temp preheat: % d \n",tempAverage);
		printf("countRepeated: % d \n",countRepeated);
		countRepeated++;
	}

	//heat up the illumination
	illuminationEnable();
	i2cCalibReg[0]=0x03;
	i2c(deviceAddress, 'w', SHUTTER_CONTROL, 1, &i2cCalibReg);
	sleep(2);
	i2cCalibReg[0]=0x00;
	i2c(deviceAddress, 'w', SHUTTER_CONTROL, 1, &i2cCalibReg);

	free(i2cCalibReg);

	return 1;
}

int16_t* calibrationLoadTemperatureDRNU(){
	char*   line = NULL;
	size_t  len = 0;
	ssize_t read;
	char fileName[64];

	sprintf(fileName, "epc660_temperature_drnu_W%03i_C%03i", configGetWaferID(), configGetChipID());
	if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
		sprintf(fileName, "epc635_temperature_drnu_W%03i_C%03i", configGetWaferID(), configGetChipID());
	}
	configAddEndFileName(fileName, "csv");

	FILE *file = fopen(fileName, "r");
	if (file == NULL){
		printf("Can not open %s file\n", fileName);
		return NULL;
	}

	printf("%s\n", fileName);

	gTempCal = 0;

	for (int k=0; k < gNumDist; k++){
		if((read = getline(&line, &len, file)) != -1){
			char *pch = strtok(line,"\n");
			gTemperatureDRNU[k] = helperStringToInteger(pch);
			printf("gTemperatureDRNU %d \n",gTemperatureDRNU[k]);
			gTempCal += gTemperatureDRNU[k];
		}
	}

	fclose(file);

	gTempCal /= gNumDist;
	printf("gTempCal= %d\n", gTempCal);
	return gTemperatureDRNU;
}

int calibrationSaveDRNU_LUT_TestXY(char* fileName){
	int x, y, k;
	FILE *file = fopen(fileName, "w");
	if (file == NULL){
		return -1;
	}

	for (y = 0; y < gHeight; y++){
		for (x = 0; x < gWidth; x++){
			for (k=0; k < gNumDist-1; k++){
				fprintf(file,"%d ", drnuLut[k][y][x]);
			}
			fprintf(file,"%d\n", drnuLut[k][y][x]);
		}
	}

	fclose(file);
	return 0;
}

int calibrationSaveDRNU_LUT_TestYX(char* fileName){
	int x, y, k;
	FILE *file = fopen(fileName, "w");
	if (file == NULL){
		return -1;
	}

	for(x = 0; x < gWidth; x++){
		for(y = 0; y < gHeight; y++){
			for(k=0; k < gNumDist-1; k++){
				fprintf(file,"%d ", drnuLut[k][y][x]);
			}
			fprintf(file,"%d\n", drnuLut[k][y][x]);
		}
	}

	fclose(file);
	return 0;
}

int calibrationLoadDRNU_LUT(){
	int x, y, k, l;
	char*   line = NULL;
	size_t  len = 0;
	ssize_t read;
	char fileName[64];
	calibration_t  calibrationMode;
	//int DRNUtalbeIndex = 0; //TODO remove

	for(int calibrationNR=0; calibrationNR < 9; calibrationNR++ ){

		int index=0;

		sprintf(fileName, "epc660_drnu_W%03i_C%03i", configGetWaferID(), configGetChipID());
		if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
			sprintf(fileName, "epc635_drnu_W%03i_C%03i", configGetWaferID(), configGetChipID());
		}

		switch(calibrationNR){
		case 0:
			strcat(fileName,"_20.csv");
			printf("try freqMhz 20\n");
			calibrationMode = DRNU_20_24MHZ_wABS_wPdelay_coarse;
			index=0;
			break;
		case 1:
			strcat(fileName,"_10.csv");
			printf("try freqMhz 10\n");
			calibrationMode = DRNU_10_12MHZ_wABS_wPdelay_coarse;
			index=1;
			break;
		case 6:
			strcat(fileName,"_Calfreq6.csv");
			calibrationMode = DRNU_20_24MHZ_wABS_wPdelay_fine;
			printf("try freq6\n");
			index=6;
			break;
		case 7:
			strcat(fileName,"_Calfreq7.csv");
			printf("try freq7\n");
			calibrationMode = NONE;
			index=7;
			break;
		case 8:
			strcat(fileName,"_Calfreq6_woABS.csv");
			calibrationMode = DRNU_20_24MHZ_woABS_wPdelay_fine;
			printf("try freq6 wo abs\n");
			index=6;		//over write drnu correction table calibration index 6
			break;
		default:
			calibrationMode = NONE;
			index=calibrationNR;
			break;
		}

		FILE *file = fopen(fileName, "r");
		if (file == NULL){
			printf("Can not open %s file\n", fileName);
			printf("Try the next file \n");

			if(index==6 && (getModulationFrequencyCalibration(index)!=NONE)){
				// do nothing	because there is only cal6 with ABS
			}else{
				setModulationFrequencyCalibration(NONE,index);
			}

			//return -1;
		}else{
			printf("%s\n", fileName);


			printf("calibrationLoadDRNU_LUT  index = %d calibrationMode = %d \n", index, calibrationMode); //TODO remove



			setModulationFrequencyCalibration(calibrationMode,index);

			if((read = getline(&line, &len, file)) != -1){
				char *pch = strtok(line,"\n");
				gOffsetDRNU_Temp[index] = helperStringToInteger(pch);
				printf("index: %d offset %d \n", index, gOffsetDRNU_Temp[index]);
			}

			if((read = getline(&line, &len, file)) != -1){
				char *pch = strtok(line,"\n");
				gTempCal_Temp[index] = helperStringToInteger(pch);
				printf("index: %d offset %d \n", index, gTempCal_Temp[index]);
			}

			if((read = getline(&line, &len, file)) != -1){
				char *pch = strtok(line,"\n");
				gStepCalMM_Temp[index] = helperStringToDouble(pch);
				printf("index: %d offset %2.4f \n", index, gStepCalMM_Temp[index]);
			}

			for (l=0, k=0; k < gNumDist; k++){
				for(y=0; y<gHeight; y++){
					if((read = getline(&line, &len, file)) != -1){
						char *pch = strtok(line," ");
						for (x=0; (pch != NULL) && (x<gWidth); x++, l++){
							if (x > gWidth){
								printf("File does not contain a proper DRNU table coefficients\n");
								return -1;
							}

							drnuLutTemp[index][l] = helperStringToInteger(pch);
							pch = strtok(NULL," ");
						}	// end for
					} else {
						printf("error calibrationLoadDRNU_LUT\n");
					}
				}
			}
			fclose(file);
		}

	}

	for(int showCalidx = 0;showCalidx<8;showCalidx++){
		printf("modulation frequency: %d calibration %d \n", showCalidx, getModulationFrequencyCalibration(showCalidx));
	}

	printf("FILE LOAD OK\n");

	int indx = 0;
	indx=configGetModFreqMode();
	calibrationChangeDRNU_LUT(indx);

	uint16_t maxDistanceCM = gSpeedOfLightDiv2 / (10 * configGetModulationFrequency(configGetDeviceAddress()));
	//double boxLSB = DRNU_CALIBRATION_BOX_LENGTH_CM * MAX_PHASE / maxDistanceCM;  //DRNU_CALIBRATION_BOX_LENGTH_CM = 34.5 cm
	double boxLSB = (double)calibrateGetCalibrationBoxLengthMM() * MAX_PHASE / (maxDistanceCM * 10.0);  //lsi 180302
	int value = boxLSB - gOffsetDRNU;
	print(true, 2, "offsetDRNU= %d  maxDistanceCM= %d  boxLSB= %2.4f newOffsetLSB= %d gStepCalMM = %2.4f\n", gOffsetDRNU, maxDistanceCM, boxLSB, value, gStepCalMM);
	calibrationSetDefaultOffsetPhase(value);
	return 0;
}


void calibrationChangeDRNU_LUT(int i){

	int l=0;
	for (int k=0; k < gNumDist; k++){
		for(int y=0; y<gHeight; y++){
			for(int x=0; x<gWidth; x++, l++){
				drnuLut[k][y][x] = drnuLutTemp[i][l];
			}
		}
	}

	gTempCal    = gTempCal_Temp[i];
	gStepCalMM  = gStepCalMM_Temp[i];
	gOffsetDRNU = gOffsetDRNU_Temp[i];
}

double calibrationFindLine(int indexEnd){
	int k, x, y;
	double avr1 = 0.0;
	double avr2 = 0.0;
	double numPixel = gHeight * gWidth;
	for(k=0, y=0; y<gHeight; y++){
		for(x=0; x<gWidth; x++){
			avr1 += drnuLut[k][y][x];
		}
	}
	avr1 /= numPixel;
	for(k=indexEnd, y=0; y<gHeight; y++){
		for(x=0; x<gWidth; x++){
			avr2 += drnuLut[k][y][x];
		}
	}
	avr2 /= numPixel;
	return ((avr2 - avr1) / (indexEnd+1));
}

void calibrationLinearRegresion(double* slope, double *offset){
	int i, j, x, y;
	double sumX  = 0;
	double sumY  = 0;
	double sumXX = 0;
	double sumXY = 0;
	for(x=0; x<gNumDist; x++){
		for(j=0; j<gHeight; j++){
			for(i=0; i<gWidth; i++){
				y = drnuLut[x][j][i];
				sumX  +=   x;
				sumXX += (x*x);
				sumY  +=   y;
				sumXY += (x*y);
			}
		}
	}
	double num = gNumDist * gWidth * gHeight;
	double m   = (num * sumXY - sumX*sumY)/(num*sumXX - sumX*sumX);
	double b   = (sumY - m * sumX) / num;
	*slope     = m;
	*offset    = b;
}

void calibrationCorrectDRNU_LUT(double slope, double offset){
	for(int k=0; k<gNumDist; k++){
		for(int y=0; y<gHeight; y++){
			for(int x=0; x<gWidth; x++){
				drnuLut[k][y][x] = drnuLut[k][y][x] - (k * slope + offset);
			}
		}
	}
}

void calibrationCorrectDRNU_Error(){
	for(int y=0; y<gHeight; y++){
		for(int x=0; x<gWidth; x++){
			int y0 = drnuLut[47][y][x];
			int y2 = drnuLut[49][y][x];
			drnuLut[48][y][x] = (y0+y2)/2;
		}
	}
}

int calibrationErrorCurveEndIndex(){
	return (round(15000/gStepCalMM));
}

int calibrationSaveDistanceImages(){
	char fileName [64];

	sprintf(fileName, "epc660_images_drnu_W%03i_C%03i", configGetWaferID(), configGetChipID());
	if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
		sprintf(fileName, "epc635_images_drnu_W%03i_C%03i", configGetWaferID(), configGetChipID());
	}

	configAddEndFileName(fileName, "bin");

	FILE *file = fopen(fileName, "wb");
	if (file == NULL){
		return -1;
	}

	printf("%s\n", fileName);

	int indx=configGetModFreqMode();
	fwrite(distImagesDRNU[indx], sizeof(uint16_t), (gHeight * gWidth * gNumDist), file);


	fclose(file);
	calibrationSaveDistanceImagesTXT();
	return 0;
}


int calibrationSaveDistanceImagesTXT(){
	int k, x, y, l;
	char fileName[64];

	sprintf(fileName, "epc660_images_drnu_W%03i_C%03i", configGetWaferID(), configGetChipID());
	if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
		sprintf(fileName, "epc635_images_drnu_W%03i_C%03i", configGetWaferID(), configGetChipID());
	}

	configAddEndFileName(fileName, "csv");

	FILE *file = fopen(fileName, "w"); //"rb"
	if (file == NULL){
		return -1;
	}

	printf("%s\n", fileName);

	int i=configGetModFreqMode();
	/*if(configGetModFreqMode() == MOD_FREQ_10000KHZ){
		i=1;
	}*/

	for(l=0, k=0; k< gNumDist; k++){
		for(y=0; y< gHeight; y++){
			for(x=0; x< gWidth; x++, l++){
				fprintf(file, "%d ", distImagesDRNU[i][l]);
			}
			fprintf(file, "\n");
		}
		fprintf(file, "\n");
	}

	fclose(file);
	return 0;
}

int calibrationLoadDistanceImages(int index){
	char fileName [64];
	calibration_t  calibrationMode;

	sprintf(fileName, "epc660_images_drnu_W%03i_C%03i", configGetWaferID(), configGetChipID());
	if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
		sprintf(fileName, "epc635_images_drnu_W%03i_C%03i", configGetWaferID(), configGetChipID());
	}

	switch(index){
		case 0:
			strcat(fileName, "_20.bin");
			calibrationMode = DRNU_20_24MHZ_wABS_wPdelay_coarse;
			break;
		case 1:
			strcat(fileName, "_10.bin");
			calibrationMode = DRNU_10_12MHZ_wABS_wPdelay_coarse;
			break;
		case 6:
			strcat(fileName, "_Calfreq6.bin");
			calibrationMode = DRNU_20_24MHZ_wABS_wPdelay_fine;
			break;
		case 7:
			strcat(fileName, "_Calfreq7.bin");
			calibrationMode = NONE;
			break;
		case 0x16:
			strcat(fileName, "_Calfreq6_woABS.bin");
			calibrationMode = DRNU_20_24MHZ_woABS_wPdelay_fine;
			index = 6;
			break;
		default:
			calibrationMode = NONE;
			break;
	}

	FILE *file = fopen(fileName, "rb");
	if (file == NULL){
		printf("Can not open %s file\n", fileName);
		setModulationFrequencyCalibration(NONE,index);
		return -1;
	}else{
		setModulationFrequencyCalibration(calibrationMode,index);
	}


	printf("%s\n", fileName);
	fread(distImagesDRNU[index], sizeof(uint16_t), (gHeight * gWidth * gNumDist), file);
	fclose(file);
	return 0;
}

int16_t* calibrateRenewDRNU(int stepMM){


	int indx = configGetModFreqMode();


	if(getModulationFrequencyCalibration(indx)==DRNU_20_24MHZ_woABS_wPdelay_fine){
		calibrationLoadDistanceImages(0x16);
	}else{
		calibrationLoadDistanceImages(indx);
	}

	calibrationDRNU_FromFile(stepMM);
	calibrationLoadTemperatureDRNU();
	for(int i=0; i<50; i++)
		gData[i] = gDrnuLutAverage[i];

	for(int i=0; i<50; i++)
		gData[50+i] = gTemperatureDRNU[i];

	return gData;
}

int16_t* calibrateShowDRNU(){
	//int numPix = gHeight * gWidth;
	int32_t drnuLutAverage[50];
	int topX,topY,bottomX,bottomY;

	if((configGetPartType() == EPC635) || (configGetPartType() == EPC503)){ // offsetRoi Calc
		topX=0;
		bottomX=gWidth;
		topY=0;
		bottomY=gHeight;
	}else{	//660 ROI DRNU offset calc
		/*int topX=50;
		int topY=50;
		int bottomX=270;
		int bottomY=190;*/
		topX=0;
		bottomX=gWidth;
		topY=0;
		bottomY=gHeight;
	}


	for(int k=0; k < gNumDist; k++){
		drnuLutAverage[k] = 0;
		int counter = 0;

		//if(getCalibNearRangeStatus()==1){
			for(int y=topY; y<bottomY; y++){
				for(int x = topX; x<bottomX; x++){
					drnuLutAverage[k] += drnuLut[k][y][x];
					counter++;
				}
			}
		/*}else{
			for(int y=0; y<gHeight; y++){
				for(int x = 0; x<gWidth; x++){
					drnuLutAverage[k] += drnuLut[k][y][x];
					counter++;
				}
			}
		}*/
		gDrnuLutAverage[k]= (int16_t)(round(drnuLutAverage[k] / counter));
	}

	calibrationLoadTemperatureDRNU();

	for(int i=0; i<50; i++){
		gData[i] = gDrnuLutAverage[i];
	}

	for(int i=0; i<50; i++){
		gData[50+i] = gTemperatureDRNU[i];
	}

	return gData;
}

void calibrationUpdateFrequency(){
	int indx = 0;
	/*if( configGetModFreqMode() == MOD_FREQ_10000KHZ ){
		indx = 1;
	}*/
	indx = configGetModFreqMode();
	printf("modFreqMode: %d \"n",configGetModFreqMode());
	printf("modFreq: %d \n",configGetModulationFrequency(configGetDeviceAddress()));
	calibrationChangeDRNU_LUT(indx);
	uint16_t maxDistanceCM = gSpeedOfLightDiv2 / (10 * configGetModulationFrequency(configGetDeviceAddress()));
	//double boxLSB = DRNU_CALIBRATION_BOX_LENGTH_CM * MAX_PHASE / maxDistanceCM;  //DRNU_CALIBRATION_BOX_LENGTH_CM = 34.5 cm
	double boxLSB = (double)calibrateGetCalibrationBoxLengthMM() * MAX_PHASE / ( maxDistanceCM * 10.0); //lsi 180302
	int value = boxLSB - gOffsetDRNU;
	print(true, 1, "offsetDRNU= %d  maxDistanceCM= %d  boxLSB= %2.4f newOffsetLSB= %d gStepCalMM = %2.4f\n", gOffsetDRNU, maxDistanceCM, boxLSB, value, gStepCalMM);
	calibrationSetOffsetPhase(value);	//sho -> boxLSB
	calibrationSetDefaultOffsetPhase(value);

}

int calibrateGetoffsetPhaseDefaultFPN(){
	return offsetPhaseDefaultFPN;
}

void calibrateSetDRNUCalibrateDelay(int time_uS){
	gDRNUCalibrateDelay = time_uS;
}

void calibrateSetDRNUDiffTemp(int diffTemp){
	gDiffTemp = diffTemp;
}

void calibrateSetDRNUAverage(int numAveragedFrames){
	gNumAveragedFrames = numAveragedFrames;
}
int  calibrateGetDRNUAverage(){
	return gNumAveragedFrames;
}

void calibrateSetpreHeatTemp(int tempPreheat){
	gPreheatTemp = tempPreheat;
}
int  calibrateGetpreHeatTemp(){
	return gPreheatTemp;
}

int calibrateGetDRNUCalibrateDelay(){
	return gDRNUCalibrateDelay;
}

int calibrateGetDRNUDiffTemp(){
	return gDiffTemp;
}

int calibrateGetTempCalib(){
	return gTempCal;
}

double calibrateGetStepCalMM(){
	return gStepCalMM;
}

void calibrateSetNumDistances(int val){
	gNumDist = val;
}

int calibrateGetNumDistances(){
	return gNumDist;
}

void calibrateSetWidth(int val){
	gWidth = val;
}

int calibrateGetWidth(){
	return gWidth;
}

void calibrateSetHeight(int val){
	gHeight = val;
}

int calibrateGetHeight(){
	return gHeight;
}

uint16_t calibrateGetDistImagePixel(int indx){
	int freq = configGetModFreqMode();

	return distImagesDRNU[freq][indx];
}

int calibrateGetDRNULutPixel(int indx, int y, int x){
	return drnuLut[indx][y][x];
}

double calibrateGetTempCoef(int partType){
	if (partType == EPC635 || partType == EPC503) {
		return gEpc635_TempCoef;
	} else {
		return gEpc660_TempCoef;
	}
}

void calibrateSetTempCoef(double koef, int partType){
	if (partType == EPC635 || partType == EPC503) {
		gEpc635_TempCoef = koef;
	} else {
		gEpc660_TempCoef = koef;
	}
}

void calibrateSetSpeedOfLightDiv2(double val){
	gSpeedOfLightDiv2 = val;
}

double calibrateGetSpeedOfLightDiv2(){
	return gSpeedOfLightDiv2;
}

void calibrateSetCalibrationBoxLengthMM(int value){
	gCalibrationBoxLengthMM = value;
}

int calibrateGetCalibrationBoxLengthMM(){
	return gCalibrationBoxLengthMM;
}

/*void saveCalibrateReg(int corrTableIdx){	//toDo Code in a calib regConfigFile

	unsigned char *i2cCalibReg = malloc(1* sizeof(unsigned char));
	int deviceAddress = configGetDeviceAddress();


	i2c(deviceAddress, 'r', DEMODULATION_DELAYS, 1, &i2cCalibReg);
	corrIdxDLLAndDemodDelaysettings[corrTableIdx][0]=i2cCalibReg[0];

	i2c(deviceAddress, 'r', DLL_FINE_CTRL_EXT_HI, 1, &i2cCalibReg);
	corrIdxDLLAndDemodDelaysettings[corrTableIdx][1]=i2cCalibReg[0];
	i2c(deviceAddress, 'r', DLL_FINE_CTRL_EXT_LO, 1, &i2cCalibReg);
	corrIdxDLLAndDemodDelaysettings[corrTableIdx][2]=i2cCalibReg[0];
	i2c(deviceAddress, 'r', DLL_FINE_CTRL_EXT_HI, 1, &i2cCalibReg);
	corrIdxDLLAndDemodDelaysettings[corrTableIdx][3]=i2cCalibReg[0];

	free(i2cCalibReg);
}

void setCalibrateReg(int corrTableIdx){	//toDo Code in a calib regConfigFile

	unsigned char *i2cCalibReg = malloc(1* sizeof(unsigned char));
	int deviceAddress = configGetDeviceAddress();

	i2cCalibReg[0]=corrIdxDLLAndDemodDelaysettings[corrTableIdx][0];
	i2c(deviceAddress, 'w', DEMODULATION_DELAYS, 1, &i2cCalibReg);

	i2cCalibReg[0]=corrIdxDLLAndDemodDelaysettings[corrTableIdx][1];
	i2c(deviceAddress, 'w', DLL_FINE_CTRL_EXT_HI, 1, &i2cCalibReg);
	i2cCalibReg[0]=corrIdxDLLAndDemodDelaysettings[corrTableIdx][2];
	i2c(deviceAddress, 'w', DLL_FINE_CTRL_EXT_LO, 1, &i2cCalibReg);
	i2cCalibReg[0]=corrIdxDLLAndDemodDelaysettings[corrTableIdx][3];
	i2c(deviceAddress, 'w', DLL_FINE_CTRL_EXT_HI, 1, &i2cCalibReg);

	free(i2cCalibReg);
}*/

/// @}
