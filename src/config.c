#define _GNU_SOURCE
#include "config.h"
#include "i2c.h"
#include "boot.h"
#include <unistd.h>
#include "evalkit_illb.h"
#include "dll.h"
#include "read_out.h"
#include "registers.h"
#include "seq_prog_loader.h"
#include "api.h"
#include "calibration.h"
#include "temperature.h"
#include "pru.h"
#include "flim.h"
#include "image_correction.h"
#include "calculation.h"
#include "calculator.h"
#include "saturation.h"
#include "adc_overflow.h"
#include "fp_atan2_lut.h"
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "helper.h"
#include "logger.h"
#include "is5351A_clkGEN.h"

/// \addtogroup configuration
/// @{

//GLOBAL VAR DECLARATION START***********************************************************************
typedef struct{
	calibration_t calibration;
	int height;
	int widght;
}calibration_param_t;


typedef struct{								//includes chip: mode, ID, image dimensions...
//device type and ID
	chip_type_t chip_type;

	unsigned int icType;
	unsigned int partType;
	unsigned int partVersion;
	uint16_t waferID;
	uint16_t chipID;
	uint16_t sequencerVersion;

	unsigned int devAddress;

//runtime mode
	op_mode_t currentMode;

//runtime settings
	mod_freq_t currentFreq;
	int numberOfFrequencies;
	uint16_t modulationFrequency;
	int modulationFrequencies[8];
	calibration_param_t calibrations[8];

	int integrationTime2D;
	int integrationTime3D;
	int integrationTime3DHDR;
	unsigned char nDCSTOF;

	int absEnabled;
	int isFLIM;

//calibration/correction settings
	int correctionBGMode;

	int enabledDRNU;
	int enabledGrayscale;
	int enabledTemperatureCorrection;
	double chipTempSimpleKalmanK;
	int enabledAmbientLightCorrection;

	int gNearRange;


	int enableSquarDcsAdd;
	int16_t nFilterLoop;

	int enableAddArg;
	int dcsAdd_threshold;
	int dcsAdd_argMin;
	int dcsAdd_tIntMin;
	int dcsAdd_argMax;
	int dcsAdd_tIntMax;
	double threshold_gain;

	uint16_t bitMask;

//print to terminal
	int gPrint;

//save trimming values for temperature readout
	unsigned char gX;
	unsigned char gY;
	unsigned char gM;
	unsigned char gC;
	unsigned char gL;
	int			  gZ;
}chip_t;


//GLOBAL VAR DECLARATION START***********************************************************************

//sequencer file name
	char seq635FileName[64];
	char seq660FileName[64];

//GLOBAL VAR INITALISATION START*********************************************************************

static chip_t chip = {	.currentMode = GRAYSCALE,
						.numberOfFrequencies=NumberOfTypes,
						.integrationTime2D = 5000,
						.integrationTime3D = 700,
						.integrationTime3DHDR = 700,
						.nDCSTOF = 4,
						.absEnabled = 1,
						.isFLIM = 0,
						.correctionBGMode = 0,
						.calibrations[0]={.calibration=NONE},
						.calibrations[1]={.calibration=NONE},
						.calibrations[2]={.calibration=NONE},
						.calibrations[3]={.calibration=NONE},
						.calibrations[4]={.calibration=NONE},
						.calibrations[5]={.calibration=NONE},
						.calibrations[6]={.calibration=NONE},
						.calibrations[7]={.calibration=NONE},
						.enableSquarDcsAdd=0,
						.nFilterLoop=0,
						.enableAddArg=0,
						.dcsAdd_threshold=8000,
						.dcsAdd_argMin=80,
						.dcsAdd_tIntMin=150,
						.dcsAdd_argMax=280,
						.dcsAdd_tIntMax=2000,
						.threshold_gain=0.10811,
						.enabledDRNU=0,
						.enabledGrayscale = 0,
						.enabledTemperatureCorrection = 1,
						.chipTempSimpleKalmanK=0.1,
						.enabledAmbientLightCorrection = 0,
						.gNearRange=1,
						.gPrint = 0};

//GLOBAL VAR INITALISATION END*********************************************************************

//LOCAL FUN DECLARTATION START*********************************************************************
void printChipParam(void){
	printf("*** device type and IDs: \n");
		printf("chip_type: %d \n",	 			chip.chip_type);
		printf("icType: %d \n", 				chip.icType);
		printf("partVersion: %d \n", 			chip.partVersion);
		printf("waferID: %d \n", 				chip.waferID);
		printf("chipID: %d \n", 				chip.chipID);
		printf("sequencerVersion: %d \n", 		chip.sequencerVersion);
		printf("devAddress: 0x%x \n", 			chip.devAddress);
		printf("currentMode: %d \n", 			chip.currentMode);

		printf("currentFreq: %d \n", 			chip.currentFreq);
		printf("modulationFrequency: %d \n", 	chip.modulationFrequency);



		//for(int i=0; i < (sizeof(chip.modulationFrequencies)/4); i++){
	    for(int i=0; i < configGetModFreqCount(); i++){
			printf("modulationFrequencies[%d]: %d \n", 		i,chip.modulationFrequencies[i]);
		}


	printf("*** runtime settings: \n");
		printf("integrationTime2D: %d \n", 		chip.integrationTime2D);
		printf("integrationTime3D: %d \n", 		chip.integrationTime3D);
		printf("integrationTime3DHDR: %d \n", 	chip.integrationTime3DHDR);
		printf("nDCSTOF: %d \n", 				chip.nDCSTOF);
		printf("absEnabled: %d \n", 			chip.absEnabled);
		printf("isFLIM: %d \n", 				chip.isFLIM);

	printf("*** calibration/correction settings: \n");
		printf("correctionBGMode: %d \n", 					chip.correctionBGMode);
		printf("enabledDRNU: %d \n", 						chip.enabledDRNU);
		printf("enabledGrayscale: %d \n", 					chip.enabledGrayscale);
		printf("enabledTemperatureCorrection: %d \n", 		chip.enabledTemperatureCorrection);
		printf("enabledAmbientLightCorrection: %d \n", 		chip.enabledAmbientLightCorrection);


	//print to terminal
	printf("*** Print level: \n");
		printf("gPrint: %d \n", 	chip.gPrint);

	//save trimming values for temperature readout
	printf("*** trimming values for temperature readout level: \n");
		printf("gX: 0x%x \n", 	chip.gX);
		printf("gY: 0x%x \n", 	chip.gY);
		printf("gM: %d \n", 	chip.gM);
		printf("gC: %d \n", 	chip.gC);
		printf("gL: %d \n", 	chip.gL);
		printf("gZ: %d \n", 	chip.gZ);
}

//LOCAL FUN DECLARTATION END**********************************************************************


/*!
 Initializes the configuration.
 @param deviceAddress i2c address of the chip (default 0x20)
 @returns On success, 0, on error -1
 */
int configInit(int deviceAddress) {
	unsigned char *i2cData = malloc(16 * sizeof(unsigned char));

	memcpy(seq660FileName, "epc660_Seq_Prog-V8.bin", 23);
	memcpy(seq635FileName, "epc635_Seq_Prog-V9.bin", 23);

	if ( configLoadParameters() == -1 ){
		configSaveParameters();
	};

	if (configGetPartType() == EPC635 || configGetPartType() == EPC503){
		chip.sequencerVersion = sequencerProgramLoad(seq635FileName, deviceAddress);
	} else {
		chip.sequencerVersion = sequencerProgramLoad(seq660FileName, deviceAddress);
	}

	i2cData[0] = 0x04;
	i2c(deviceAddress, 'w', DLL_MEASUREMENT_RATE_LO, 1, &i2cData); // Set DLL measurement rate to 4.
	i2cData[0] = 0xCC;
	i2c(deviceAddress, 'w', LED_DRIVER, 1, &i2cData); // Enable LED preheat.

	// new implementation for temperature sensor readout
	i2c(deviceAddress, 'r', 0xD3, 1, &i2cData);
	chip.gX = i2cData[0];

	i2c(deviceAddress, 'r', 0xD5, 1, &i2cData);
	chip.gY = i2cData[0];

	i2c(deviceAddress, 'r', 0x92, 1, &i2cData);
	chip.gM = i2cData[0];

	i2cData[0] = 0x00;
	i2c(deviceAddress, 'w', 0x3A, 1, &i2cData);	//grayscale differential readout

	i2c(deviceAddress, 'r', 0xE8, 1, &i2cData);
	chip.gC = i2cData[0];

	chip.gZ = -272; //0x110 default value

	if(chip.gC != 0xFF){
		chip.gZ = ((double)chip.gC / 4.68) - 299; //0x12B;
	}

	i2cData[0] = 0x03;
	i2c(deviceAddress, 'w', SEQ_CONTROL, 1, &i2cData); // Enable pixel and TCMI sequencers.

	mod_freq_t freq = MOD_FREQ_10000KHZ;
	configFindModulationFrequencies();
	configSetModulationFrequency(freq, deviceAddress);

	// Check if FLIM chip:
	if (configGetPartType() == EPC502_OBS || configGetPartType() == EPC502 || configGetPartType() == EPC503) {
		chip.isFLIM = 1;
	} else {
		chip.isFLIM = 0;
	}

	dllDetermineAndSetDemodulationDelay(deviceAddress);
	dllBypassDLL(1, deviceAddress); // bypass DLL
	if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
		readOutSetROI(4, 163, 6, 65, deviceAddress);
		calibrateSetHeight(60);
		calibrateSetWidth(160);
	} else {
		readOutSetROI(4, 323, 6, 125, deviceAddress);
		calibrateSetHeight(240);
		calibrateSetWidth(320);
	}

	if(chip.isFLIM == 0){
		calibrateSetNumDistances(50); //for DRNU
	}

	temperatureInit(deviceAddress); //
	op_mode_t mode;
	pruInit(deviceAddress);
	if (chip.isFLIM) {
		printf("FLIM enabled\n");
		mode = FLIM;
		// FLIM needs 20Mhz
		mod_freq_t freq = MOD_FREQ_20000KHZ;
		configSetModulationFrequency(freq, deviceAddress);
		// set delay to 0, not sure what's better
		// dllResetDemodulationDelay(deviceAddress);
		flimSetT1(2);
		flimSetT2(1);
		flimSetT3(1);
		flimSetT4(4);
		flimSetTREP(6);
		flimSetRepetitions(1000);
		flimSetFlashDelay(1);
		flimSetFlashWidth(1);
	} else { // TOF
		mode = THREED;
	}
	configInitRegisters(mode);
	imageCorrectionLoad("image_correction.txt");
	imageCorrectionTransform();
	configLoad(mode, deviceAddress);

	if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
		calculatorSetPixelMask(0xFFFF);
		chip.bitMask = 0xFFFF;
		saturationSetSaturationBit(0x0001);
		adcOverflowSetMin(16);
		adcOverflowSetMax(65519-16);
		i2cData[0] = 0x63;
		i2c(deviceAddress, 'w', I2C_TCMI_CONTROL, 1, &i2cData); // Enable tcmi_8bit_data_sat_en.
		calibrationSetNumberOfColumnsMax(168);
		calibrationSetNumberOfRowsMax(72);
	}else{
		chip.bitMask = 0x0FFF;
	}

	free(i2cData);

	if (chip.isFLIM == 0){

		chip.enabledGrayscale = 1;
		if(calculateLoadGrayscaleGainOffset() < 0){
			chip.enabledGrayscale = 0;
		}

		chip.enabledDRNU = 1;
		if(calibrationLoadDRNU_LUT() < 0){
			chip.enabledDRNU = 0;
		}

		chip.enabledAmbientLightCorrection = 1;

		calibrationLoadDistanceImages(MOD_FREQ_20000KHZ);	//images DRNU 20MHz
		calibrationLoadDistanceImages(MOD_FREQ_10000KHZ);	//images DRNU 10MHz


		if(calibrationLoadDistanceImages(0x16)==-1){		//MOD_FREQ_20000KHZFace & 0x10 correciton without ABS
			calibrationLoadDistanceImages(MOD_FREQ_20000KHZFace);
		}

		calibrationLoadDistanceImages(MOD_FREQ_36000KHZ);	//images DRNU 36MHz


		calculationInitKalman();

	} else {	//FLIM
		chip.enabledGrayscale = 0;
		chip.enabledDRNU = 0;
		chip.enabledAmbientLightCorrection = 0;

		calculateLoadFlimGainOffset(0);
		calculateLoadFlimGainOffset(1);
	}

	printChipParam();

	if (chip.isFLIM == 0){
		atan2lut_Init();
	}

	return 0;
}

/*!
 Loads the configuration corresponding to the specified mode.
 @param mode
 @param deviceAddress i2c address of the chip (default 0x20)
 @returns On success, 0, on error -1
 */
int configLoad(const op_mode_t mode, const int deviceAddress) {
	unsigned char *i2cData = malloc(16 * sizeof(unsigned char));
	switch (mode) {
	case THREED:
		configSetDCS(chip.nDCSTOF, deviceAddress);
		if (chip.absEnabled) {
			if (calculationIsHDR()) {
				i2cData[0] = 0x30;
			} else {
				i2cData[0] = 0x34;
			}
		} else {
			if (calculationIsHDR()) {
				i2cData[0] = 0x00;
			} else {
				i2cData[0] = 0x04;
			}
		}
		i2c(deviceAddress, 'w', MT_0_HI, 1, &i2cData); // differential readout
		i2cData[0] = 0x00;
		i2c(deviceAddress, 'w', MT_0_LO, 1, &i2cData); // LED modulated, modulation gates not forced
		chip.currentMode = THREED;
		if (calculationIsHDR()) {
			configSetIntegrationTimesHDR();
		} else {
			configSetIntegrationTime3D(chip.integrationTime3D, deviceAddress);
		}
		break;

	case GRAYSCALE:
	default:
		i2cData[0] = 0x00;
		i2c(deviceAddress, 'w', SHUTTER_CONTROL, 1, &i2cData); // disable shutter
		//i2cData[0] = 0x04;
		i2c(deviceAddress, 'r', MOD_CONTROL, 1, &i2cData);
		i2cData[0] &= 0xCF;
		//i2cData[0] &= 0xCF;
		i2c(deviceAddress, 'w', MOD_CONTROL, 1, &i2cData); // only 1 DCS frame
		i2cData[0] = 0x00;  //0x14; - old
		i2c(deviceAddress, 'w', MT_0_HI, 1, &i2cData); // differential readout MGA
		i2cData[0] = 0x26;
		i2c(deviceAddress, 'w', MT_0_LO, 1, &i2cData); // suppress the LED, force MGA high and MGB low during integration
		chip.currentMode = GRAYSCALE;
		configSetIntegrationTime2D(chip.integrationTime2D, deviceAddress);
		break;
	case FLIM:
		i2cData[0] = 0x94;
		i2c(deviceAddress, 'w', MOD_CONTROL, 1, &i2cData); // FLIM, 2 DCSs
		chip.currentMode = FLIM;
		break;
	}

	free(i2cData);
	return 0;
}

/*!
 Getter function for the mode
 @returns mode
 */
op_mode_t configGetCurrentMode() {
	return chip.currentMode;
}

/*!
 Sets the integration time for the grayscale mode
 @param us integration time in microseconds
 @param deviceAddress i2c address of the chip (default 0x20)
 @returns On success, 0, on error -1
 */
int configSetIntegrationTime2D(const int us, const int deviceAddress) {

	if (us < 0 || us > MAX_INTEGRATION_TIME_2D) {
		return -1;
	}
	if (chip.currentMode == GRAYSCALE) {
		if (configSetIntegrationTime(us, deviceAddress, 0xA2, 0xA0) == 0) {
			chip.integrationTime2D = us;
			return 0;
		}
	} else {
		chip.integrationTime2D = us;
		return 0;
	}
	return -1;
}

/*!
 Sets the integration time for the TOF mode
 @param us integration time in microseconds
 @param deviceAddress i2c address of the chip (default 0x20)
 @returns On success, 0, on error -1
 */
int configSetIntegrationTime3D(const int us, const int deviceAddress) {

	if (us < 0 || us > MAX_INTEGRATION_TIME_3D) {
		return -1;
	}

	if (calculationIsHDR()) {
		chip.integrationTime3D = us;
		configSetIntegrationTimesHDR();
	} else if (chip.currentMode == THREED) {
		if (configSetIntegrationTime(us, deviceAddress, 0xA2, 0xA0) == 0) {
			chip.integrationTime3D = us;
			if(us > chip.dcsAdd_tIntMin){ //calculate new threshold
				chip.dcsAdd_threshold = (int)(chip.dcsAdd_argMin+(us-chip.dcsAdd_tIntMin)*chip.threshold_gain);
				//printf("newThreshold: %d \n gain: %2.4f",chip.dcsAdd_threshold,chip.threshold_gain);
			}else{
				chip.dcsAdd_threshold = chip.dcsAdd_argMin;
			}
		}
	} else {
		chip.integrationTime3D = us;

		if(us > chip.dcsAdd_tIntMin){ //calculate new threshold
			chip.dcsAdd_threshold = (int)(chip.dcsAdd_argMin+(us-chip.dcsAdd_tIntMin)*chip.threshold_gain);
			//printf("newThreshold: %d \n gain: %2.4f",chip.dcsAdd_threshold,chip.threshold_gain);
		}else{
			chip.dcsAdd_threshold = chip.dcsAdd_argMin;
		}
	}

	double factor = calculationGetAmbientLightFactorOrg() / sqrt( chip.integrationTime3D );

	calculationSetAmbientLightFactor(factor);
	return 0;
}

/*!
 Sets the integration time for the HDR mode
 @param us integration time in microseconds
 @param deviceAddress i2c address of the chip (default 0x20)
 @returns On success, 0, on error -1
 */
int configSetIntegrationTime3DHDR(const int us, const int deviceAddress) {
	if (us < 0 || us > MAX_INTEGRATION_TIME_3D) {
		return -1;
	}

	chip.integrationTime3DHDR = us;
	configSetIntegrationTimesHDR();
	return 0;
}

/*!
 Get Imaging time .
 */
int16_t configGetImagingTime(){
	return 15000;
}

int configEnableSquareAddDcs(const int squareAddSquareDcs){
	if(squareAddSquareDcs!=0){
		chip.nFilterLoop=1;
		chip.enableSquarDcsAdd=squareAddSquareDcs;
	}else{
		chip.nFilterLoop=0;
		chip.enableSquarDcsAdd=squareAddSquareDcs;
	}
	return squareAddSquareDcs;
}

int configStatusEnableSquareAddDcs(){
	return chip.enableSquarDcsAdd;
};


int16_t setNfilterLoop(int16_t nLoop){
	if(0 <= nLoop && nLoop <= 100)
		chip.nFilterLoop=nLoop;
	else
		chip.nFilterLoop=100;
	return chip.nFilterLoop;
};

int16_t getNfilterLoop(){
	return chip.nFilterLoop;
};

int configEnableAddArg(const int enableAddArg){
	chip.enableAddArg=enableAddArg;
	return chip.enableAddArg;
}

int configStatusEnableAddArg(){
	return chip.enableAddArg;
}

int configSetDcsAdd_threshold(const int thresholdAddArg){
	chip.dcsAdd_threshold=thresholdAddArg;
	return chip.dcsAdd_threshold;
}


int configGetDcsAdd_threshold(){
	return chip.dcsAdd_threshold;
}


int configSetAddArgMin(const int argMin){
	chip.dcsAdd_argMin=argMin;
	chip.dcsAdd_tIntMin=chip.integrationTime3D;

	chip.threshold_gain=(double)(chip.dcsAdd_argMax-chip.dcsAdd_argMin)/(chip.dcsAdd_tIntMax-chip.dcsAdd_tIntMin);
	printf("gain: %2.4f \n",chip.threshold_gain);
	return chip.dcsAdd_argMin;
}

int configSetAddArgMax(const int argMax){
	chip.dcsAdd_argMax=argMax;
	chip.dcsAdd_tIntMax=chip.integrationTime3D;

	chip.threshold_gain=(double)(chip.dcsAdd_argMax-chip.dcsAdd_argMin)/(chip.dcsAdd_tIntMax-chip.dcsAdd_tIntMin);
	printf("gain: %2.4f \n",chip.threshold_gain);
	return chip.dcsAdd_argMax;
}


/*!
 Returns the (1st) TOF integration time in us.
 @returns integration time in us
 */
int configGetIntegrationTime3D() {
	return chip.integrationTime3D;
}

/*!
 Returns the (2nd) HDR TOF integration time in us.
 @returns integration time in us
 */
int configGetIntegrationTime3DHDR() {
	return chip.integrationTime3DHDR;
}

/*!
 Returns the Grayscale integration time in us.
 @returns integration time in us
 */
int configGetIntegrationTime2D() {
	return chip.integrationTime2D;

}

/*!
 Sets the integration time for the given mode
 @param us integration time in microseconds
 @param deviceAddress i2c address of the chip (default 0x20)
 @param valueAddress register for the value
 @param loopAddress register for the loop
 @returns On success, 0, on error -1
 */
int configSetIntegrationTime(const int us, const int deviceAddress,
	const int valueAddress, const int loopAddress) {
	unsigned char *i2cData = malloc(16 * sizeof(unsigned char));
	uint8_t intmHi;
	uint8_t intmLo;
	uint8_t intLenHi;
	uint8_t intLenLo;

	double tModClk = configGetTModClk();

	//printf("modulationFreq: %d\n",chip.modulationFrequency);
	if(chip.modulationFrequency>24000){
		tModClk=tModClk*96/(chip.modulationFrequency*4/1000);
		printf("tModClk: %f\n",tModClk);
	}

	double max = 0x10000 * tModClk / 1000.0;

	if (us <= max) {
		intmHi = 0x00;
		intmLo = 0x01;

		if (us <= 0) {
			intLenHi = 0x00;
			intLenLo = 0x00;
		} else {
			int intLen = us * 1000 / tModClk - 1;
			intLenHi = (intLen >> 8) & 0xFF;
			intLenLo = intLen & 0xFF;
		}
	} else {
		int intm = us / max + 1;
		intmHi = (intm >> 8) & 0xFF;
		intmLo = intm & 0xFF;
		//TODO: -0.5 because of round?
		int intLen = (double) us / intm * 1000.0 / tModClk - 1;
		intLenHi = (intLen >> 8) & 0xFF;
		intLenLo = intLen & 0xFF;
	}

	i2cData[0] = (unsigned char) intmHi;
	i2cData[1] = (unsigned char) intmLo;
	i2c(deviceAddress, 'w', loopAddress, 2, &i2cData);

	i2cData[0] = (unsigned char) intLenHi;
	i2cData[1] = (unsigned char) intLenLo;
	i2c(deviceAddress, 'w', valueAddress, 2, &i2cData);

	free(i2cData);

	return 0;
}

void configSetIntegrationTimesHDR() {
	//TODO: more precise
	unsigned char *i2cData = malloc(2 * sizeof(unsigned char));
	unsigned int deviceAddress = configGetDeviceAddress();
	double tModClk = configGetTModClk();
	double max = 0x10000 * tModClk / 1000.0;

	int intm0 = chip.integrationTime3D / max + 1;
	int intm1 = chip.integrationTime3DHDR / max + 1;
	int intm = intm0;
	uint8_t intmHi;
	uint8_t intmLo;
	int intLen0;
	int intLen1;
	uint8_t intLenHi0;
	uint8_t intLenLo0;
	uint8_t intLenHi1;
	uint8_t intLenLo1;

	if (intm1 > intm) {
		intm = intm1;
	}
	intmHi = (intm >> 8) & 0xFF;
	intmLo = intm & 0xFF;
	if (chip.integrationTime3D <= 0) {
		intLenHi0 = 0x00;
		intLenLo0 = 0x00;
	} else {
		intLen0 = (double) chip.integrationTime3D / intm * 1000.0 / tModClk - 1;
		intLenHi0 = (intLen0 >> 8) & 0xFF;
		intLenLo0 = intLen0 & 0xFF;
	}

	if (chip.integrationTime3DHDR <= 0) {
		intLenHi1 = 0x00;
		intLenLo1 = 0x00;
	} else {
		intLen1 = (double) chip.integrationTime3DHDR / intm * 1000.0 / tModClk - 1;
		intLenHi1 = (intLen1 >> 8) & 0xFF;
		intLenLo1 = intLen1 & 0xFF;
	}

	i2cData[0] = (unsigned char) intmHi;
	i2cData[1] = (unsigned char) intmLo;
	i2c(deviceAddress, 'w', INTM_HI, 2, &i2cData);

	i2cData[0] = (unsigned char) intLenHi0;
	i2cData[1] = (unsigned char) intLenLo0;
	i2c(deviceAddress, 'w', INT_LEN_HI, 2, &i2cData);

	i2cData[0] = (unsigned char) intLenHi1;
	i2cData[1] = (unsigned char) intLenLo1;
	i2c(deviceAddress, 'w', INT_LEN_MGX_HI, 2, &i2cData);

	free(i2cData);
}

/*!
 Sets the integration time for the given mode
 @param us integration time in microseconds
 @param deviceAddress i2c address of the chip (default 0x20)
 @param valueAddress register for the value
 @param loopAddress register for the loop
 @returns On success, 0, on error -1
 */
int configSetModulationFrequency(const mod_freq_t freq, const int deviceAddress) {
	unsigned char *i2cData = malloc(16 * sizeof(unsigned char));
	switch (freq) {
	case MOD_FREQ_20000KHZ:
		i2cData[0] = 0x00;
		chip.modulationFrequency = chip.modulationFrequencies[0];
		break;
	case MOD_FREQ_5000KHZ:
		i2cData[0] = 0x03;
		chip.modulationFrequency = chip.modulationFrequencies[2];
		break;
	case MOD_FREQ_2500KHZ:
		i2cData[0] = 0x07;
		chip.modulationFrequency = chip.modulationFrequencies[3];
		break;
	case MOD_FREQ_1250KHZ:
		i2cData[0] = 0x0F;
		chip.modulationFrequency = chip.modulationFrequencies[4];
		break;
	case MOD_FREQ_625KHZ:
		i2cData[0] = 0x1F;
		chip.modulationFrequency = chip.modulationFrequencies[5];
		break;
	case MOD_FREQ_20000KHZFace:
		chip.modulationFrequency = chip.modulationFrequencies[6];
		break;
	case MOD_FREQ_36000KHZ:
		chip.modulationFrequency = chip.modulationFrequencies[7];
		break;
	case MOD_FREQ_10000KHZ:
	default:
		i2cData[0] = 0x01;
		chip.modulationFrequency = chip.modulationFrequencies[1];
		break;
	}

	if(freq!=MOD_FREQ_36000KHZ && freq!=MOD_FREQ_20000KHZFace){
		chip.currentFreq = freq;
		i2c(deviceAddress, 'w', MOD_CLK_DIVIDER, 1, &i2cData);
		i2cData[0] = 0x3f;
		i2c(deviceAddress, 'w', CLK_ENABLES, 1, &i2cData);
		i2cData[0] = 0x00;
		i2c(deviceAddress, 'w', DLL_FINE_CTRL_EXT_HI, 1, &i2cData);
		i2c(deviceAddress, 'w', DLL_FINE_CTRL_EXT_LO, 1, &i2cData);
		i2cData[0] = 0x00;
		i2c(deviceAddress, 'w', DLL_COARSE_CTRL_EXT, 1, &i2cData);

		//setting ISOURCE to default EEPROM value
		i2cData[0] = ISOURCE_CLK_DIVIDER;
		i2c(deviceAddress, 'w', EE_ADDR, 1, &i2cData);
		i2c(deviceAddress, 'r', EE_DATA, 1, &i2cData);
		i2c(deviceAddress, 'w', ISOURCE_CLK_DIVIDER, 1, &i2cData);

		//setting coarse step calibration
		setCalibNearRangeStatus(0);
	}else{
		if(freq==MOD_FREQ_20000KHZFace){
			i2cData[0] = 0x00;
			i2c(deviceAddress, 'w', MOD_CLK_DIVIDER, 1, &i2cData);

			i2cData[0] = ISOURCE_CLK_DIVIDER;
			i2c(deviceAddress, 'w', EE_ADDR, 1, &i2cData);
			i2c(deviceAddress, 'r', EE_DATA, 1, &i2cData);
			i2c(deviceAddress, 'w', ISOURCE_CLK_DIVIDER, 1, &i2cData);

			//initClkGen(30);
			//i2cData[0] = 0x7f;
			//i2c(deviceAddress, 'w', CLK_ENABLES, 1, &i2cData);
			chip.currentFreq = freq;
			/*i2cData[0] = 0x03;											//3 DLL setting (good phase start)
			i2c(deviceAddress, 'w', DLL_COARSE_CTRL_EXT, 1, &i2cData);
			i2cData[0] = 0x1a;											//speed up ISOURCE clk
			i2c(deviceAddress, 'w', ISOURCE_CLK_DIVIDER, 1, &i2cData);*/
			//setting coarse step calibration
			setCalibNearRangeStatus(1);
		}else if(freq==MOD_FREQ_36000KHZ){
			i2cData[0] = 0x00;
			i2c(deviceAddress, 'w', MOD_CLK_DIVIDER, 1, &i2cData);

			i2cData[0] = ISOURCE_CLK_DIVIDER;
			i2c(deviceAddress, 'w', EE_ADDR, 1, &i2cData);
			i2c(deviceAddress, 'r', EE_DATA, 1, &i2cData);
			i2c(deviceAddress, 'w', ISOURCE_CLK_DIVIDER, 1, &i2cData);

			initClkGen(36);
			i2cData[0] = 0x7f;
			i2c(deviceAddress, 'w', CLK_ENABLES, 1, &i2cData);
			chip.currentFreq = freq;
			i2cData[0] = 0x00;											//3 DLL setting (good phase start)
			i2c(deviceAddress, 'w', DLL_COARSE_CTRL_EXT, 1, &i2cData);
			i2cData[0] = 0x1a;											//speed up ISOURCE clk
			i2c(deviceAddress, 'w', ISOURCE_CLK_DIVIDER, 1, &i2cData);
			//setting coarse step calibration
			setCalibNearRangeStatus(1);
		}
	}
	free(i2cData);
	configSetIntegrationTime2D(chip.integrationTime2D, deviceAddress);
	configSetIntegrationTime3D(chip.integrationTime3D, deviceAddress);
	return 0;
}

/*!
 Getter function for the modulation frequency
 @param deviceAddress i2c address of the chip (default 0x20)
 @returns the modulation frequency in kHz
 */
uint16_t configGetModulationFrequency(const int deviceAddress) {
	print(true, 2, "---- modulation frequency = %i ----\n", chip.modulationFrequency);
	return chip.modulationFrequency;
}

mod_freq_t configGetModFreqMode() {
	return chip.currentFreq;
}

//in khz
void configFindModulationFrequencies() {
	int sysClk = configGetSysClk();
	printf("sysClk = %i\n", sysClk);
	int i;
	int modFrequency;
	for (i = 0; i < configGetModFreqCount(); i++) {
		if(i<MOD_FREQ_20000KHZFace){
			int modFrequency = sysClk / (4 * (pow(2, i)));
			chip.modulationFrequencies[i] = modFrequency;
			printf("%i frequency -> %i\n", i, modFrequency);
		}else if(i==MOD_FREQ_20000KHZFace){
			modFrequency=chip.modulationFrequencies[0];
			chip.modulationFrequencies[i] = modFrequency;
			printf("%i frequency -> %i\n", i, modFrequency);
		}else if(i==MOD_FREQ_36000KHZ){
			modFrequency=36000;
			chip.modulationFrequencies[i] = modFrequency;
			printf("%i frequency -> %i\n", i, modFrequency);
		}
	}
}

int* configGetModulationFrequencies() {
	return chip.modulationFrequencies;
}

void setModulationFrequencyCalibration(calibration_t calibrationMode,mod_freq_t freq){
	if (freq<sizeof(chip.calibrations)){
		//printf("freq index: %d set new calibrationMode: %d \n",freq,calibrationMode);
		chip.calibrations[freq].calibration=calibrationMode;
	}else{
		//printf("frequency index out of range \n");
	}

}

calibration_t getModulationFrequencyCalibration(mod_freq_t freq){
	calibration_t returnCalibStatus=NONE;

	if (freq<configGetModFreqCount()){
		returnCalibStatus=chip.calibrations[freq].calibration;
		//printf("frequency: %d set calibration: %d \n",chip.modulationFrequencies[freq],returnCalibStatus);
	}
	return returnCalibStatus;
}

/*!
 Enables or disables ABS
 @param state 1 for enable, 0 for disable
 @param deviceAddress i2c address of the chip (default 0x20)
 */
void configEnableABS(const unsigned int state, const int deviceAddress) {
	unsigned char *i2cData = malloc(1 * sizeof(unsigned char));
	if (state != 0) {
		chip.absEnabled = 1;
		i2c(deviceAddress, 'r', MT_0_HI, 1, &i2cData);
		i2cData[0] |= 0x30;
		i2c(deviceAddress, 'w', MT_0_HI, 1, &i2cData);

		i2c(deviceAddress, 'r', MT_1_HI, 1, &i2cData);
		i2cData[0] |= 0x30;
		i2c(deviceAddress, 'w', MT_1_HI, 1, &i2cData);

		i2c(deviceAddress, 'r', MT_2_HI, 1, &i2cData);
		i2cData[0] |= 0x30;
		i2c(deviceAddress, 'w', MT_2_HI, 1, &i2cData);

		i2c(deviceAddress, 'r', MT_3_HI, 1, &i2cData);
		i2cData[0] |= 0x30;
		i2c(deviceAddress, 'w', MT_3_HI, 1, &i2cData);
	} else {
		chip.absEnabled = 0;
		i2c(deviceAddress, 'r', MT_0_HI, 1, &i2cData);
		i2cData[0] &= 0xCF;
		i2c(deviceAddress, 'w', MT_0_HI, 1, &i2cData);

		i2c(deviceAddress, 'r', MT_1_HI, 1, &i2cData);
		i2cData[0] &= 0xCF;
		i2c(deviceAddress, 'w', MT_1_HI, 1, &i2cData);

		i2c(deviceAddress, 'r', MT_2_HI, 1, &i2cData);
		i2cData[0] &= 0xCF;
		i2c(deviceAddress, 'w', MT_2_HI, 1, &i2cData);

		i2c(deviceAddress, 'r', MT_3_HI, 1, &i2cData);
		i2cData[0] &= 0xCF;
		i2c(deviceAddress, 'w', MT_3_HI, 1, &i2cData);
	}
	free(i2cData);
}

int configIsABS(const int deviceAddress) {
	/*unsigned char *i2cData = malloc(1 * sizeof(unsigned char));
	i2c(deviceAddress, 'r', MT_0_HI, 1, &i2cData);
	int enabled = (i2cData[0] & 0x30) == 0x30;
	free(i2cData);*/
	return chip.absEnabled;
}


uint16_t getConfigBitMask(){
	return chip.bitMask;
}

void setConfigBitMask(uint16_t mask){
	chip.bitMask = mask;
}


/*!
 Sets the number of DCSs
 @param nDCS number of DCSs
 @param deviceAddress i2c address of the chip (default 0x20)
 */
void configSetDCS(unsigned char nDCS, const int deviceAddress) {
	chip.nDCSTOF = nDCS;
	nDCS = (nDCS - 1) << 4;
	unsigned char *i2cData = malloc(1 * sizeof(unsigned char));
	i2c(deviceAddress, 'r', MOD_CONTROL, 1, &i2cData);
	i2cData[0] |= nDCS; //set 1
	i2cData[0] &= (nDCS | 0xCF); //set 0
	i2c(deviceAddress, 'w', MOD_CONTROL, 1, &i2cData);
	free(i2cData);
}

void configSetMode(const int select, const int deviceAddress) {
	unsigned char *i2cData = malloc(1 * sizeof(unsigned char));
	i2c(deviceAddress, 'r', MOD_CONTROL, 1, &i2cData);
	if (select) {
		i2cData[0] |= 0x40;
	} else {
		i2cData[0] &= 0xBF;
	}
	i2c(deviceAddress, 'w', MOD_CONTROL, 1, &i2cData);
	free(i2cData);
}

int configGetModFreqCount() {
	return chip.numberOfFrequencies;
}

int configSetModFreqCount(int numberFrequencies) {
	chip.numberOfFrequencies = numberFrequencies;
	printf("numberOfFrequencies set to: %d \n",chip.numberOfFrequencies);
	return chip.numberOfFrequencies;
}

int configGetModeCount() {
	return NumberOfModes;
}

void configSetDeviceAddress(const int deviceAddress) {
	chip.devAddress = deviceAddress;
}

int configGetDeviceAddress() {
	return chip.devAddress;
}

int configIsFLIM() {
	return chip.isFLIM;
}

int configGetSysClk() {
	unsigned char *i2cData = malloc(1 * sizeof(unsigned char));
	int clockKhz = 4000; //clock in kHz
	i2c(chip.devAddress, 'r', PLL_FB_DIVIDER, 1, &i2cData);
	int pllFbDivider = (i2cData[0]) & 0x1F;
	i2c(chip.devAddress, 'r', PLL_PRE_POST_DIVIDERS, 1, &i2cData);
	int pllPostDivider = (i2cData[0] >> 4) & 0x03;
	free(i2cData);
	return (clockKhz * (pllFbDivider + 1)) >> pllPostDivider;
}

double configGetTModClk() {
	unsigned char *i2cData = malloc(1 * sizeof(unsigned char));
	i2c(chip.devAddress, 'r', MOD_CLK_DIVIDER, 1, &i2cData);
	int modClkDivider = i2cData[0] & 0x1F;
	free(i2cData);
	return 1000000.0 / (configGetSysClk() / (modClkDivider + 1.0));
}

double configGetTPLLClk() {
	unsigned char *i2cData = malloc(1 * sizeof(unsigned char));
	int clockKhz = 4000; //clock in kHz
	i2c(chip.devAddress, 'r', PLL_FB_DIVIDER, 1, &i2cData);
	int pllFbDivider = (i2cData[0]) & 0x1F;
	i2c(chip.devAddress, 'r', PLL_PRE_POST_DIVIDERS, 1, &i2cData);
	int pllPostDivider = (i2cData[0] >> 4) & 0x03;
	int sysClk = (clockKhz * (pllFbDivider + 1)) >> pllPostDivider;
	free(i2cData);
	return 1000000.0 / sysClk;
}

void configSetIcVersion(unsigned int value) {
	chip.icType = value;
}

unsigned int configGetIcVersion() {
	return chip.icType;
}

void configSetPartType(unsigned int value) {
	chip.partType = value;

	chip.chip_type = ((chip.partType*10)+chip.partVersion);
}

void configSetPartVersion(unsigned int value) {
	chip.partVersion = value;

	chip.chip_type = ((chip.partType*10)+chip.partVersion);
}

unsigned int configGetPartVersion() {
	return chip.partVersion;
}

unsigned int configGetPartType() {
	return chip.partType;
}

void configSetWaferID(const uint16_t ID) {
	chip.waferID = ID;
}

uint16_t configGetWaferID() {
	return chip.waferID;
}

void configSetChipID(const uint16_t ID) {
	chip.chipID = ID;
}

uint16_t configGetChipID() {
	return chip.chipID;
}

uint16_t configGetSequencerVersion(){
	return chip.sequencerVersion;
}

unsigned char configGetGX(){
	return chip.gX;
}

unsigned char configGetGY(){
	return chip.gY;
}

int configGetGZ(){
	return chip.gZ;
}

unsigned char configGetGM(){
	return chip.gM;
}

unsigned char configGetGL(){
	return chip.gL;
}

unsigned char configGetGC(){
	return chip.gC;
}


int configIsEnabledDRNUCorrection(){
	return chip.enabledDRNU;
}

int configIsEnabledGrayscaleCorrection(){
	return chip.enabledGrayscale;
}

int configIsEnabledTemperatureCorrection(){
	return chip.enabledTemperatureCorrection;
}

double setChipTempSimpleKalmanK(double K){
	chip.chipTempSimpleKalmanK = K;
	return chip.chipTempSimpleKalmanK;
}

double getChipTempSimpleKalmanK(){
	return chip.chipTempSimpleKalmanK;
}

int configIsEnabledAmbientLightCorrection(){
	return chip.enabledAmbientLightCorrection;
}

int setCalibNearRangeStatus(int status){
	chip.gNearRange=status;
	return status;
}

int getCalibNearRangeStatus(){
	return chip.gNearRange;
}

int configPrint(){
	return chip.gPrint;
}

int configSetPrint(int val){
	chip.gPrint = val;
	return chip.gPrint;
}

int configSaveParameters(){
	FILE    *file;
	file = fopen("./config.txt", "w");
	if (file == NULL){
		printf("Could not open config.txt file.\n");
		return -1;
	}

	printf("ambientLightFactor= %f\n", calculationGetAmbientLightFactorOrg());
	fprintf(file, "%f\n", calculationGetAmbientLightFactorOrg());

	printf("printVal= %d\n", configPrint());
	fprintf(file, "%d\n", configPrint());

	printf("diffTemp= %d\n", calibrateGetDRNUDiffTemp());
	fprintf(file, "%d\n", calibrateGetDRNUDiffTemp());

	printf("drnuDelay= %d\n", calibrateGetDRNUCalibrateDelay());
	fprintf(file, "%d\n", calibrateGetDRNUCalibrateDelay());

	printf("numDRNUAveragedFrames= %d\n", calibrateGetDRNUAverage());
	fprintf(file, "%d\n", calibrateGetDRNUAverage());

	printf("epc635_TemperatureCoeff= %2.4f\n", calibrateGetTempCoef(EPC635));
	fprintf(file, "%2.6f\n", calibrateGetTempCoef(EPC635));

	printf("epc660_TemperatureCoeff= %2.4f\n", calibrateGetTempCoef(EPC660));
	fprintf(file, "%2.6f\n", calibrateGetTempCoef(EPC660));

	printf("speed of light div 2= %2.2f\n", calibrateGetSpeedOfLightDiv2());
	fprintf(file, "%2.2f\n", calibrateGetSpeedOfLightDiv2());

	printf("calibration box length mm= %d\n", calibrateGetCalibrationBoxLengthMM());
	fprintf(file, "%d\n", calibrateGetCalibrationBoxLengthMM());

	printf("epc660 sequencer: %s\n", seq660FileName);
	fprintf(file, "%s\n", seq660FileName);

	printf("epc635 sequencer: %s\n", seq635FileName);
	fprintf(file, "%s\n", seq635FileName);

	fclose (file);
	return 0;
}

int configLoadParameters(){
	char    *line = NULL;
	size_t   len = 0;
	ssize_t  read;
	FILE    *file;
	double factor = 0;
	double dval;
	int val, c;

	file = fopen("./config.txt", "r");
	if (file == NULL){
		printf("Could not open config.txt file.\n");
		return -1;
	}

	for(int i=0; (read = getline(&line, &len, file)) != -1; i++){
		char *pch = strtok(line," ");

		switch(i){
		case 0:
			factor = helperStringToDouble(pch);
			printf("ambientLightFactor = %2.4f\n", factor);
			calculationSetAmbientLightFactorOrg(factor);
			break;
		case 1:
			val = helperStringToInteger(pch);
			printf("print = %d\n", val);
			configSetPrint(val);
			break;
		case 2:
			val = helperStringToInteger(pch);
			printf("diffTemp = %d\n", val);
			calibrateSetDRNUDiffTemp(val);
			break;
		case 3:
			val = helperStringToInteger(pch);
			printf("drnuDelay = %d\n", val);
			calibrateSetDRNUCalibrateDelay( val) ;
			break;
		case 4:
			val = helperStringToInteger(pch);
			printf("numAveragedFrames = %d\n", val);
			calibrateSetDRNUAverage(val);
			break;
		case 5:
			dval = helperStringToDouble(pch);
			printf("epc635_TemperatureCoeff = %2.6f\n", dval);
			calibrateSetTempCoef(dval, EPC635);
			break;
		case 6:
			dval = helperStringToDouble(pch);
			printf("epc660_TemperatureCoeff = %2.6f\n", dval);
			calibrateSetTempCoef(dval, EPC660);
			break;
		case 7:
			dval = helperStringToDouble(pch);
			printf("speed of light div 2 = %2.2f\n",dval);
			calibrateSetSpeedOfLightDiv2(dval);
			break;
		case 8:
			val = helperStringToInteger(pch);
			printf("calibration box length mm = %d\n", val);
			calibrateSetCalibrationBoxLengthMM(val);
			break;
		case 9:
			for(c=0; pch[c] != '\0' && pch[c] != '\n' && c < 64; c++ ) seq660FileName[c] = pch[c];
			printf("epc660 sequencer: %s\n", seq660FileName);
			break;
		case 10:
			for(c=0; pch[c] != '\0' && pch[c] != '\n' && c < 64; c++ ) seq635FileName[c] = pch[c];
			printf("epc635 sequencer: %s\n", seq635FileName);
			break;
		}	//end switch
	}	// end for

	fclose (file);
	double newFactor = factor / sqrt( configGetIntegrationTime3D() );
	calculationSetAmbientLightFactor(newFactor);
	return 0;
}


int configRestartTOF(){
	printf("---------------restart TOF--------------\n");
	int deviceAddress = configGetDeviceAddress();
	unsigned char *i2cData = malloc(1 * sizeof(unsigned char));
	i2cData[0] = 0xfa;

	if(configGetPartType() == EPC502){
		i2cData[1] = 0x02; //epc660
		i2c(deviceAddress, 'w', EE_ADDR, 2, &i2cData);
	}else if(configGetPartType() == EPC503){
		i2cData[1] = 0x04; //epc635
		i2c(deviceAddress, 'w', EE_ADDR, 2, &i2cData);
	}

	free(i2cData);

	bootReset();

	configSetPartType(bootGetPartType(deviceAddress));		// Read PART_TYPE
	configSetPartVersion(bootGetPartVersion(deviceAddress));
	configSetIcVersion(bootGetIcVersion(deviceAddress)); 	// Read IC_VERSION
	configSetWaferID(bootGetWaferID(deviceAddress));		// Read WAFER_ID
	configSetChipID(bootGetChipID(deviceAddress));			// Read CHIP_ID
	bootNormalMode(deviceAddress);
	bootCloseEpc660_enable();

	illuminationDisable();
	configInit(deviceAddress);
	illuminationEnable();
	return 0;
}



int configRestartFLIM(){
	printf("------------------restart FLIM---------------------\n");
	int deviceAddress = configGetDeviceAddress();
	unsigned char *i2cData = malloc(2 * sizeof(unsigned char));
	i2cData[0] = 0xfa;

	if(configGetPartType() == EPC660){
		i2cData[1] = 0x03;	//epc502
		i2c(deviceAddress, 'w', EE_ADDR, 2, &i2cData);
	}else if(configGetPartType() == EPC635){
		i2cData[1] = 0x05;	//epc503
		i2c(deviceAddress, 'w', EE_ADDR, 2, &i2cData);
	}


	free(i2cData);

	bootReset();

	configSetPartType(bootGetPartType(deviceAddress));	// Read PART_TYPE:
	configSetPartVersion(bootGetPartVersion(deviceAddress));
	configSetIcVersion(bootGetIcVersion(deviceAddress)); // Read IC_VERSION:
	configSetWaferID(bootGetWaferID(deviceAddress)); // Read WAFER_ID:
	configSetChipID(bootGetChipID(deviceAddress)); // Read CHIP_ID:

	bootNormalMode(deviceAddress);
	bootCloseEpc660_enable();

	illuminationDisable();
	configInit(deviceAddress);
	illuminationEnable();

	return 0;
}


void configInitRegisters(op_mode_t mode){
	int deviceAddress = configGetDeviceAddress();
	unsigned char *i2cData = malloc(1 * sizeof(unsigned char));

	if(mode == FLIM && chip.partVersion == 7 && chip.partType == EPC502){
		i2cData[0] = 0x0f;
		i2c(deviceAddress, 'w', 0xd1, 1, &i2cData);
		i2cData[0] = 0x0f;
		i2c(deviceAddress, 'w', 0xd8, 1, &i2cData);
		i2cData[0] = 0x3f;
		i2c(deviceAddress, 'w', 0x88, 1, &i2cData);
	}else if(mode == FLIM){
		i2cData[0] = 0x3f;
		i2c(deviceAddress, 'w', 0x88, 1, &i2cData);
	}

	free(i2cData);
}



int print(bool val, int a, char* format, ...){
	int ret = 0 ;
	if(chip.gPrint >= a){
		if(val){
			va_list args;
			char buffer[512];
			va_start(args, format);
			ret = vsprintf(buffer, format, args);
			printf("%s", buffer);
			va_end(args);
		}
	}

	return ret;
}

/// @}
