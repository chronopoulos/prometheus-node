#include "temperature.h"
#include "registers.h"
#include "config.h"
#include "pru.h"
#include "i2c.h"
#include "bbb_adc.h"
#include "api.h"
#include "calculation.h"
#include "evalkit_constants.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

/// \addtogroup temperature
/// @{

static int fdAIN2;
static int fdAIN4;
static int tempChip = 0;
static int tempIllumination = 0;
static uint16_t gTemperatureGrayscaleData[MAX_NUM_PIX];

static unsigned char x, y, v, w, m;
static unsigned char arrayC[4];
static int16_t arrayZ[4];


/*!
 Init for the ADCs
 @note has to be called before temperatureGetTemparature() or temperatureGetTemperatureIllumination()
 */
void temperatureInit(const int deviceAddress) {
	fdAIN2 = adcOpen(2);
	fdAIN4 = adcOpen(4);
}

/*!
 Reads the temperature values out of the ROI (dummy rows) or registers if available and the ADCs connected to the illumination board.
 @param deviceAddress
 @params pointer to the array of temperatures
 @note check apiGetTemperature for more Information
 */
int16_t temperatureGetTemperatures(const int deviceAddress, int16_t *temps) {
	int size = temperatureGetTemperatureChip(deviceAddress, temps);
	size += temperatureGetTemperatureIllumination(temps + size);
	return size;
}

int16_t temperatureGetTemperature(const int deviceAddress){
	int16_t temps[4];
	temperatureGetTemperatures(deviceAddress, temps);
	int temperature = 0;
	if (configGetPartType() == EPC660 || configGetPartType() == EPC502){
		for(int j=0; j<4; j++)
			temperature += temps[j];

		temperature = (int)((round)((double)temperature/4.0));
	} else {
		temperature = temps[0];
	}

	return temperature;
}

/*!
 Reads the temperature values of the Chip.
 @param deviceAddress
 @params pointer to the array of temperatures
 @note check apiGetTemperature for more Information
 */
int16_t temperatureGetTemperatureChip(const int deviceAddress, int16_t *temps){

	if (configGetPartType() == EPC660 || configGetPartType() == EPC502){

		if( configGetPartVersion() >= 7){
			return temperatureGetTemperatureROI_EPC660_CHIP7(deviceAddress, temps);
			//return temperatureKalmanFilter(temps, size);
		}

		int zoom = 0;
		unsigned char *i2cData = malloc(1 * sizeof(unsigned char));
		i2c(deviceAddress, 'r', 0xD3, 1, &i2cData);

		//SHO start
		int intTime2DTemp = configGetIntegrationTime2D();
		configSetIntegrationTime2D(100, deviceAddress);
		//SHO end


		unsigned char d3;
		unsigned char d5;
		unsigned char da;
		unsigned char dc;
		if ((i2cData[0] & 0x60) != 0x60){	//zoomed
			zoom = 1;
			// Backup settings for ADC zoom:
			d3 = i2cData[0];
			i2c(deviceAddress, 'r', 0xD5, 1, &i2cData);
			d5 = i2cData[0];
			i2c(deviceAddress, 'r', 0xDA, 1, &i2cData);
			da = i2cData[0];
			i2c(deviceAddress, 'r', 0xDC, 1, &i2cData);
			dc = i2cData[0];

			// Set zooming factor to 1 (disabled):
			i2cData[0] = 0x61;
			i2c(deviceAddress, 'w', 0xD3, 1, &i2cData);
			i2c(deviceAddress, 'w', 0xDA, 1, &i2cData);
			i2cData[0] = 0x01;
			i2c(deviceAddress, 'w', 0xDC, 1, &i2cData);
			i2c(deviceAddress, 'w', 0xD5, 1, &i2cData);
		}

		uint16_t res = temperatureGetTemperatureROI(deviceAddress, temps);
		if (zoom){
			i2cData[0] = d3;
			i2c(deviceAddress, 'w', 0xD3, 1, &i2cData);
			i2cData[0] = d5;
			i2c(deviceAddress, 'w', 0xD5, 1, &i2cData);
			i2cData[0] = da;
			i2c(deviceAddress, 'w', 0xDA, 1, &i2cData);
			i2cData[0] = dc;
			i2c(deviceAddress, 'w', 0xDC, 1, &i2cData);
		}

		//SHO start
		configSetIntegrationTime2D(intTime2DTemp, deviceAddress);
		//SHO end

		free(i2cData);
		return res;
	} else {
		return temperatureGetTemperature635(deviceAddress, temps);
		//return temperatureKalmanFilter(temps, size);
	}
}

/*!
 Reads the temperature values out of Register.
 @param deviceAddress
 @params pointer to the array of temperatures
 @returns number of temp values
 @note check apiGetTemperature for more Information
 */
int16_t temperatureGetTemperatureRegister(const int deviceAddress, int16_t *temps){
	unsigned char *i2cData = malloc(8 * sizeof(unsigned char));
	unsigned int numberOfTempValues;
	double slope, offset, slopeFactor, scale;

	if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
		numberOfTempValues = 1;
		offset      = 1397.46;  // Temp. Sens. 0.5x, ADC 2.5x, IC tester @27C
		slope       = 4.6780;   // 3.7424 * 0.5 * 2.5 = 4.6780
		slopeFactor = 79.3750;
		scale       = 1.0859;
	} else {
		numberOfTempValues = 4;
		offset      = 1082.0;
		slope       = 3.7424012151;
		slopeFactor = 127.0;
		scale       = 1.0;
	}

	i2c(deviceAddress, 'r', SUM_TEMP_TL_HI, 2*numberOfTempValues, &i2cData);

	if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
		temps[0] = ((((i2cData[0]&0x3F) << 8) + i2cData[1])) / 4.0 + 0.5 - 2048;
	} else {
		temps[0] = ((((i2cData[0]&0x3F) << 8) + i2cData[1])) / 4.0 + 0.5 - 2048;
		temps[1] = ((((i2cData[2]&0x3F) << 8) + i2cData[3])) / 4.0 + 0.5 - 2048;
		temps[2] = ((((i2cData[4]&0x3F) << 8) + i2cData[5])) / 4.0 + 0.5 - 2048;
		temps[3] = ((((i2cData[6]&0x3F) << 8) + i2cData[7])) / 4.0 + 0.5 - 2048;
	}

	i2c(deviceAddress, 'r', TEMP_TL_CAL1, 8, &i2cData);
	tempChip = 0;

	for (int i = 0; i < numberOfTempValues; i++) {

		if (i2cData[2 * i] == 0xFF) {
			if (i2cData[2 * i + 1] != 0xFF) { // slope calibrated
				slope -= ((i2cData[2 * i + 1] - 127) / slopeFactor);
			}
		} else {
			offset -= ((i2cData[2 * i] - 127) / scale);
			if (i2cData[2 * i + 1] != 0xFF) { // slope and offset calibrated
				slope -= ((i2cData[2 * i + 1] - 127) / slopeFactor);
			}
		}

		temps[i] = 10 * (27 + (temps[i] - offset) / slope);	// 10 x for digit after the comma
		tempChip += temps[i];
	}

	tempChip /= (double)numberOfTempValues + 0.5;
	free(i2cData);
	return numberOfTempValues;
}

uint16_t temperatureGetTemperatureROI_EPC660_CHIP7(const int deviceAddress, int16_t *temps){


	unsigned char *i2cData = malloc(1 * sizeof(unsigned char));
	op_mode_t currentMode = configGetCurrentMode();
	int numberOfTemperatureValues = 4;

	//SHO start
	int intTime2DTemp = configGetIntegrationTime2D();
	configSetIntegrationTime2D(100, deviceAddress);
	//SHO end

	i2c(deviceAddress, 'r', 0xd3, 1, &i2cData);
	v = i2cData[0];
	i2c(deviceAddress, 'r', 0xd5, 1, &i2cData);
	w = i2cData[0];
	i2c(deviceAddress, 'r', 0xda, 1, &i2cData);
	x = i2cData[0];
	i2c(deviceAddress, 'r', 0xdc, 1, &i2cData);
	y = i2cData[0];
	i2c(deviceAddress, 'r', 0x92, 1, &i2cData);
	m = i2cData[0];

	i2cData[0] = 0x00;
	i2c(deviceAddress, 'w', 0x3a, 1, &i2cData);

	i2c(deviceAddress, 'r', 0xe8, 1, &i2cData);
	arrayC[0] = i2cData[0];
	i2c(deviceAddress, 'r', 0xea, 1, &i2cData);
	arrayC[1] = i2cData[0];
	i2c(deviceAddress, 'r', 0xec, 1, &i2cData);
	arrayC[2] = i2cData[0];
	i2c(deviceAddress, 'r', 0xee, 1, &i2cData);
	arrayC[3] = i2cData[0];

	for(int i=0; i<numberOfTemperatureValues; i++){
		arrayZ[i] = (int16_t)((double)arrayC[i] / 4.7 - 0x12B);
	}
    //SHO start
	i2cData[0] = 0x60 | v;
	i2c(deviceAddress, 'w', 0xd3, 1, &i2cData);

	i2cData[0] = 0x0f & w;
	i2c(deviceAddress, 'w', 0xd5, 1, &i2cData);

	i2cData[0] = 0x60 | x;
	i2c(deviceAddress, 'w', 0xda, 1, &i2cData);

	i2cData[0] = 0x0f & y;
	i2c(deviceAddress, 'w', 0xdc, 1, &i2cData);
    //SHO end
	if(currentMode == THREED){
		printf("load grayscale mode \n");
		apiLoadConfig(GRAYSCALE, deviceAddress);
	}

	i2cData[0] = 0xc4;
	i2c(deviceAddress, 'w', 0x92, 1, &i2cData);

	i2cData[0] = 0x26;
	i2c(deviceAddress, 'w', 0x3c, 1, &i2cData);

	uint16_t *data = NULL;

	i2cData[0] = 0xc8; //switch off illumination driver
	i2c(deviceAddress, 'w', 0x90, 1, &i2cData);

	int size = pruGetImage(&data);
	printf("size grayscale image: %d \n",size);
	printf("size grayscale gTemperatureGrayscaleData: %d \n",sizeof(gTemperatureGrayscaleData));

	i2cData[0] = 0xcc; //switch on illumination driver
	i2c(deviceAddress, 'w', 0x90, 1, &i2cData);


	for(int i=0; i< size; i++){
		gTemperatureGrayscaleData[i] = data[i];
	}

	int16_t  arrayTH[4], arrayTL[4];
	i2c(deviceAddress, 'r', 0x60, 1, &i2cData);
	arrayTH[0] = i2cData[0];
	i2c(deviceAddress, 'r', 0x61, 1, &i2cData);
	arrayTL[0] = i2cData[0];

	i2c(deviceAddress, 'r', 0x62, 1, &i2cData);
	arrayTH[1] = i2cData[0];
	i2c(deviceAddress, 'r', 0x63, 1, &i2cData);
	arrayTL[1] = i2cData[0];

	i2c(deviceAddress, 'r', 0x64, 1, &i2cData);
	arrayTH[2] = i2cData[0];
	i2c(deviceAddress, 'r', 0x65, 1, &i2cData);
	arrayTL[2] = i2cData[0];

	i2c(deviceAddress, 'r', 0x66, 1, &i2cData);
	arrayTH[3] = i2cData[0];
	i2c(deviceAddress, 'r', 0x67, 1, &i2cData);
	arrayTL[3] = i2cData[0];

	printf("numberOfTemperatureValues: %d \n",numberOfTemperatureValues);
	for(int i=0; i<numberOfTemperatureValues; i++){
		temps[i] = 10 * ((((int16_t)arrayTH[i] * 0x100 + (int16_t)arrayTL[i]) - 0x2000) * 0.134 + arrayZ[i]);
	}

	i2cData[0] = v & 0x1f;
	i2c(deviceAddress, 'w', 0xd3, 1, &i2cData);
	i2cData[0] = w | 0x30;
	i2c(deviceAddress, 'w', 0xd5, 1, &i2cData);
	i2cData[0] = x & 0x1f;
	i2c(deviceAddress, 'w', 0xda, 1, &i2cData);
	i2cData[0] = y | 0x30;
	i2c(deviceAddress, 'w', 0xdc, 1, &i2cData);
	i2cData[0] = m;
	i2c(deviceAddress, 'w', 0x92, 1, &i2cData);

	//SHO start
	configSetIntegrationTime2D(intTime2DTemp, deviceAddress);
	//SHO end

	free(i2cData);

	if(currentMode == THREED){
		apiLoadConfig(THREED, deviceAddress);
	}

	return numberOfTemperatureValues;
}



/*!
 Reads the temperature values out of the ROI (dummy rows).
 @param deviceAddress
 @params pointer to the array of temperatures
 @returns number of temperature values
 @note check apiGetTemperature for more Information
 */
uint16_t temperatureGetTemperatureROI(const int deviceAddress, int16_t *temps) {
	//backup
	op_mode_t currentMode = configGetCurrentMode();
	op_mode_t mode = GRAYSCALE;
	uint16_t numberOfTemperatureValus = 4;

	if (currentMode == THREED) {
		//device was in THREED mode, will change mode to GRAYSCALE for temperature readout
		pruSetDCS(1);
		configLoad(mode, deviceAddress);
	}
	unsigned char *i2cData = malloc(6 * sizeof(unsigned char));		//to less free???
	unsigned char backupResolutionReduction;
	unsigned char *backupROI = malloc(6 * sizeof(unsigned char));

	i2c(deviceAddress, 'r', RESOLUTION_REDUCTION, 1, &i2cData);
	backupResolutionReduction = i2cData[0];
	i2c(deviceAddress, 'r', ROI_TL_X_HI, 6, &backupROI);

	int backupNCols = pruGetNCols() * pruGetColReduction();
	int backupNRows = pruGetNRowsPerHalf() * pruGetRowReduction();

	int colReductionBkp = pruGetColReduction();
	int rowReductionBkp = pruGetRowReduction();
	if (colReductionBkp > 1) {	    
		pruSetColReduction(0);
	}
	if (rowReductionBkp > 1) {
		pruSetRowReduction(0);
	}

	i2cData[0] = 0x00;
	i2c(deviceAddress, 'w', RESOLUTION_REDUCTION, 1, &i2cData);

	if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
		pruSetSize(12, 2);
		i2cData[0] = 0x00;
		i2cData[1] = 0x9C;
		i2cData[2] = 0x00;
		i2cData[3] = 0xA7;
		i2cData[4] = 0x40;
		i2cData[5] = 0x41;
		numberOfTemperatureValus = 1;
	} else {
		pruSetSize(328, 2);
		i2cData[0] = 0x00;
		i2cData[1] = 0x00;
		i2cData[2] = 0x01;
		i2cData[3] = 0x47;
		i2cData[4] = 0x7C;
		i2cData[5] = 0x7D;
	}
	i2c(deviceAddress, 'w', ROI_TL_X_HI, 6, &i2cData);

	// get Temperature
	int16_t *pMem = NULL;
	calculationBW(&pMem);

	unsigned int offset = 8192;
	if (configIsFLIM()){
		offset = 0;
	}

	if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
		temps[0] = 0; // TODO: select correct data
	} else {
		temps[0] = (pMem[0]   + pMem[1]   + pMem[656]  + pMem[657]  - offset) / 4;
		temps[1] = (pMem[326] + pMem[327] + pMem[982]  + pMem[983]  - offset) / 4;
		temps[2] = (pMem[328] + pMem[329] + pMem[984]  + pMem[985]  - offset) / 4;
		temps[3] = (pMem[654] + pMem[655] + pMem[1310] + pMem[1311] - offset) / 4;
	}

	i2c(deviceAddress, 'r', TEMP_TL_CAL1, 8, &i2cData);

	// convert LSB to C
	int i;
	//	int calibrated;
	for (i = 0; i < numberOfTemperatureValus; i++) {
		double offset = 1082;
		double slope = 3.7424012151;
		if (i2cData[2 * i] == 0xFF) {
			if (i2cData[2 * i + 1] == 0xFF) {
				// nothing calibrated
			} else {
				// slope calibrated
				slope -= ((i2cData[2 * i + 1] - 127) / 127.0);
			}
		} else {
			offset -= ((i2cData[2 * i] - 127));
			// offset calibrated
			if (i2cData[2 * i + 1] == 0xFF) {
				// slope and offset calibrated
			} else {
				slope -= ((i2cData[2 * i + 1] - 127) / 127.0);
			}
		}
		temps[i] = 10 * (25 + (temps[i] - offset) / slope);	// 10 x for digit after the comma
		tempChip += temps[i];
	}
	tempChip /= i;
	// restore

	if (currentMode == THREED) {
		apiLoadConfig(1, deviceAddress);
	}

	if (colReductionBkp > 1) {
		pruSetColReduction(1);
	}
	if (rowReductionBkp > 1) {
		switch(rowReductionBkp) {
		case 2:
			pruSetRowReduction(1);
			break;
		case 4:
			pruSetRowReduction(2);
			break;
		case 8:
			pruSetRowReduction(3);
			break;
		default:
			printf("WARNING: Invalid reduction.");
			break;
		}
	}
	pruSetSize(backupNCols, backupNRows);

	i2cData[0] = backupResolutionReduction;
	i2c(deviceAddress, 'w', RESOLUTION_REDUCTION, 1, &i2cData);
	i2c(deviceAddress, 'w', ROI_TL_X_HI, 6, &backupROI);

	free(backupROI);
	free(i2cData);
	return numberOfTemperatureValus;
}

double temperatureGetAveragedChipTemperature(const int deviceAddress){
	int16_t temps[4];
	double avgTemp = 0.0;
	temperatureGetTemperatureROI(deviceAddress, temps);
	int i;
	for (i = 0; i < 4; i++){
		avgTemp += temps[i];
	}
	return  avgTemp / 40.0;
}

/*!
 Reads the temperature values from the ADCs.
 @params pointer to the array of temperatures
 @note check apiGetTemperature for more Information
 */
int16_t temperatureGetTemperatureIllumination(int16_t *temps) {
	double offset = 1452.13;
	double slope = 4.41;
	int measurements = 10;
	int temp0 = 0;
	int temp1 = 0;
	int i;
	for (i = 0; i < measurements; i++) {
		temp0 += adcGetValue(fdAIN4);
		temp1 += adcGetValue(fdAIN2);
	}
	temps[0] = temp0 / measurements;
	temps[1] = temp1 / measurements;
	temps[0] = 10 * (25 - (temps[0] - offset) / slope);
	temps[1] = 10 * (25 - (temps[1] - offset) / slope);
	tempIllumination = (temps[0] + temps[1]) / 2;
	return 2;
}

int16_t temperatureGetLastTempChip(){
	return tempChip;
}

int16_t temperatureGetLastTempIllumination(){
	return tempIllumination;
}

int16_t temperatureGetTemperature635(const int deviceAddress, int16_t *temps){
	unsigned char *i2cData = malloc(3 * sizeof(unsigned char));

	//SHO start
	int intTime2DTemp = configGetIntegrationTime2D();
	configSetIntegrationTime2D(100, deviceAddress);
	//SHO end

	op_mode_t currentMode = configGetCurrentMode();

	i2c(deviceAddress, 'r', 0x92, 1, &i2cData);
	m = i2cData[0];
	i2cData[0] = configGetGX() | 0x60;
	i2c(deviceAddress, 'w', 0xD3, 1, &i2cData);
	i2cData[0] = configGetGY() & 0x0F;
	i2c(deviceAddress, 'w', 0xD5, 1, &i2cData);

	if(currentMode == THREED){
		apiLoadConfig(GRAYSCALE, deviceAddress);
	}

	i2cData[0] = 0xC4;
	i2c(deviceAddress, 'w', 0x92, 1, &i2cData);

	i2cData[0] = 0x26;
	i2c(deviceAddress, 'w', 0x3C, 1, &i2cData);

	i2cData[0] = 0xc8; //switch off illumination driver
	i2c(deviceAddress, 'w', 0x90, 1, &i2cData);

	uint16_t *data = NULL;
	int size = pruGetImage(&data);

	i2cData[0] = 0xcc; //switch off illumination driver
	i2c(deviceAddress, 'w', 0x90, 1, &i2cData);

	for(int i=0; i< size; i++){
		gTemperatureGrayscaleData[i] = data[i];
	}

	i2c(deviceAddress, 'r', 0x60, 2, &i2cData);
	unsigned char TH = i2cData[0];
	unsigned char TL = i2cData[1];

	int16_t temp = ((int16_t)TH * 256 + (int16_t)TL);
	double temperature = ((double)temp - 8192) * 0.134 + configGetGZ(); //8192 -> 0x2000
	temps[0] = (int16_t)(temperature * 10.0);

	i2cData[0] = configGetGX() & 0x1F;
	i2c(deviceAddress, 'w', 0xD3, 1, &i2cData);
	i2cData[0] = configGetGY() | 0x30;
	i2c(deviceAddress, 'w', 0xD5, 1, &i2cData);
	i2cData[0] = m;
	i2c(deviceAddress, 'w', 0x92, 1, &i2cData);

	//SHO start
	configSetIntegrationTime2D(intTime2DTemp, deviceAddress);
	//SHO end

	free(i2cData);

	if(currentMode == THREED){
		apiLoadConfig(THREED, deviceAddress);
	}

	return 1; //number of temperature values
}

int16_t temperatureKalmanFilter(int16_t *temps, int size){
	static int16_t tempsOld[8];
	double k = 0.5;

	for(int i=0; i<size; i++){
		temps[i] =  temps[i] * k + (1-k) * tempsOld[i];
		tempsOld[i] = temps[i];
	}

	return  size;
}

uint16_t temperatureGetGrayscalePixel(int index){
	return gTemperatureGrayscaleData[index];
}

uint16_t* temperatureGetGrayscaleData(){
	return gTemperatureGrayscaleData;
}


/// @}
