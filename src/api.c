#include "api.h"
#include "evalkit_constants.h"
#include "config.h"
#include "pru.h"
#include "dll.h"
#include "evalkit_illb.h"
#include "i2c.h"
#include "calculation.h"
#include "read_out.h"
#include "calibration.h"
#include "calculation.h"
#include "chip_info.h"
#include "hysteresis.h"
#include "temperature.h"
#include "pn.h"
#include "image_averaging.h"
#include "image_correction.h"
#include "image_processing.h"
#include "image_difference.h"
#include "saturation.h"
#include "adc_overflow.h"
#include "is5351A_clkGEN.h"
#include "flim.h"
#include "logger.h"
#include "version.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>

/// \addtogroup api
/// @{

static unsigned int nDCSTOF = 4;
static unsigned int rowReductionBeforeBinning = 1;
unsigned char* pData;

/*!
 Sets the ROI (Region of Interest) and updates the pointers to the DDR memory
 @param topLeftX x coordinate of the top left point (0 - 322), always less than bottomRightX, must be even
 @param bottomRightX x coordinate of the bottom right point (5 - 327), always greater than topLeftX, must be odd
 @param topLeftY y coordinate of the top left point (0 - 124), always less than bottomRightY, must be even
 @param bottomRightY y coordinate of the bottom right point (1-125), always greater than topLeftY, must be odd
 @param deviceAddress i2c address of the chip (default 0x20)
 @returns On success, 0, on error -1
 @note
 usage with server:
 - set the full ROI -> setROI 0 327 0 125
 .
 default (epc660/epc502):
 - setROI 4 323 6 125
 @warning ROI can be set to min 6 by 2
 */

int16_t apiSetROI(const int topLeftX, const int bottomRightX, const int topLeftY, const int bottomRightY, const int deviceAddress) {
	if (topLeftX < 0 || topLeftX > bottomRightX || topLeftX % 2 == 1 || bottomRightX < 5 || bottomRightX >= MAX_NUMBER_OF_COLS || bottomRightX % 2 == 0) {
		printf("bad x coordinates\n");
		return -1;
	}
	if (topLeftY < 0 || topLeftY > bottomRightY || topLeftY % 2 == 1 || bottomRightY < 1 || bottomRightY >= MAX_NUMBER_OF_DOUBLE_ROWS || bottomRightY % 2 == 0) {
		printf("bad y coordinates\n");
		return -1;
	}


	int width = bottomRightX - topLeftX + 1;
	int height = bottomRightY - topLeftY + 1;
	int minWidth = 6 * pruGetColReduction();
	int minHeight = 2 * pruGetRowReduction();
	printf("width = %i, height = %i\n", width, height);
	if (width < minWidth || height < minHeight) {
		printf("size too small: min size = %i x %i\n", minWidth, minHeight);
		return -1;
	}
	readOutSetROI(topLeftX, bottomRightX, topLeftY, bottomRightY, deviceAddress);
	pruSetSize(bottomRightX - topLeftX + 1, bottomRightY - topLeftY + 1);
	imageCorrectionTransform();
	imageDifferenceInit();
	return 0;
}

/*!
 Loads a config for the mode with the given index into registers, enables or disables the illumination and updates pointers to the DDR memory
 @param configIndex 0 for GRAYSCALE, 1 for THREED, everything else results in loading the GRAYSCALE config
 @param deviceAddress I2C address of the chip (default 0x20)
 @returns index of loaded config
 @note
 usage with server:
 - loading GRAYCALE config -> loadConfig 0
 .
 default:
 - loadConfig 1 (THREED)
 */
int16_t apiLoadConfig(const int configIndex, const int deviceAddress) {
	op_mode_t mode;

	switch (configIndex) {
	case 1:
		mode = THREED;
		pruSetDCS(nDCSTOF);
		break;
	case 0:
		mode = GRAYSCALE;
		pruSetDCS(1);
		break;
	default:
		mode = GRAYSCALE;
		pruSetDCS(1);
		break;
	}
	configLoad(mode, deviceAddress);
	return configIndex;
}

/*!
 Reads Bytes from the registers over I2C
 @param registerAddress address of the register it starts to read from
 @param nBytes number of Bytes to read
 @param values pointer to the array where values will be stored to
 @param deviceAddress I2C address of the chip (default 0x20)
 @returns On success, 0, on error -1
 @note
 usage with server:
 - reading ROI (6Bytes) -> readRegister 96 6   or   read 96 6   or   r 96 6
 - reading 1 Byte from Register AE -> read AE 1   or   read AE
 */
int16_t apiReadRegister(const int registerAddress, const int nBytes, unsigned char *values, const int deviceAddress) {
	if (i2c(deviceAddress, 'r', registerAddress, nBytes, &values) > 0) {
		return nBytes;
	}
	return -1;
}

/*!
 Writes Bytes into the registers over I2C
 @param registerAddress address of the register it starts to write from
 @param nBytes number of Bytes to write
 @param values pointer to the array where values are stored in
 @param deviceAddress I2C address of the chip (default 0x20)
 @returns On success, 0, on error -1
 @note
 usage with server:
 - writing default ROI (6Bytes) -> writeRegister 96 00 04 01 43 06 70    or   write 96 ...   or   w 96 6 ...
 - writing 1 Byte to Register AE -> write AE 1   or   write AE
 */
int16_t apiWriteRegister(const int registerAddress, const int nBytes, unsigned char *values, const int deviceAddress) {
	i2c(deviceAddress, 'w', registerAddress, nBytes, &values);
	return nBytes;
}


/*!
 Enables or disables imaging
 @param state if not 0, imaging is blocked
 @returns state
 @note usage with server:
 - enabled imaging 1
 .
 default:
 - int state 1
 */
int16_t apiSetEnableImaging(const int state){
	return setEnableImaging(state);
}


/*!
 2x2 pixel square.
 @returns imagingTime
 @note
 usage with server:
 -add dcs 2x2 pixel square
 -better edge/checker smoothing
 */

int apiEnableSquareAddDcs(const int enableDcsSquare){
	return configEnableSquareAddDcs(enableDcsSquare);
}

/*!
 loops the pixelfield filter should be applied
 @returns imagingTime
 @note
 usage with server:
 -loop of pixelfield filter (f.egg. square)
 */

int16_t apiSetNfilterLoop(const int16_t nLoopApi){
	return setNfilterLoop(nLoopApi);
}

/*!
 Enables or disables the horizontal binning
 @param state 1 for enable, 0 for disable
 @param deviceAddress I2C address of the chip (default 0x20)
 @returns the state of horizontal binning
 @note
 usage with server:
 - enable horizontal binning -> enableHorizontalBinning 1
 - disable horizontal binning -> enableHorizontalBinning 0
 .
 default:
 - enableHorizontalBinning 0
 */
int16_t apiEnableHorizontalBinning(const int state, const int deviceAddress) {
	int enable = 1;
	if (!state) {
		enable = 0;
	}
	readOutEnableHorizontalBinning(enable, deviceAddress);
	pruSetColReduction(enable);
	imageCorrectionTransform();
	imageDifferenceInit();
	return enable;
}

/*!
 Enables or disables the vertical binning
 @param state 1 for enable, 0 for disable
 @param deviceAddress I2C address of the chip (default 0x20)
 @returns the state of vertical binning
 @note
 usage with server:
 - enable vertical binning -> enableVerticalBinning 1
 - disable vertical binning -> enableVerticalBinning 0
 .
 default:
 - enableVerticalBinning 0
 */
int16_t apiEnableVerticalBinning(const int state, const int deviceAddress) {
	int enable = 1;
	if (!state) {
		enable = 0;
	}
	if (enable) {
		rowReductionBeforeBinning = log2(pruGetRowReduction());
		// check if rowReduction is > 2 else set it to 2:
		if (rowReductionBeforeBinning < 2) {
			apiSetRowReduction(1, deviceAddress);
		}
	} else {
		apiSetRowReduction(0, deviceAddress);
	}
	readOutEnableVerticalBinning(enable, deviceAddress);
	imageCorrectionTransform();
	imageDifferenceInit();
	return enable;
}

/*!
 Sets the reduction of the rows
 @param value reduction factor = 2^value. Allowed values 0 - 3, other values will be changed to 0 or 3.
 @param deviceAddress I2C address of the chip (default 0x20)
 @returns the value of the row reduction
 @note
 usage with server:
 - show every row -> setRowReduction 0
 - only show every 4th row -> setRowReduction 2
 .
 default:
 - setRowReduction 0
 */
int16_t apiSetRowReduction(int value, const int deviceAddress) {
	if (value < 0) {
		value = 0;
	} else if (value > 3) {
		value = 3;
	}
	readOutSetRowReduction(value, deviceAddress);
	pruSetRowReduction(value);
	imageCorrectionTransform();
	imageDifferenceInit();
	return value;
}

/*!
 Enables or disables ABS
 @param state 1 for enable, 0 for disable
 @param deviceAddress I2C address of the chip (default 0x20)
 @returns the state of the ABS
 @note
 usage with server:
 - disable ABS -> enableABS 0
 .
 default:
 - enableABS 1
 */
int16_t apiEnableABS(const int state, const int deviceAddress) {
	configEnableABS(state, deviceAddress);
	imageDifferenceInit();
	return state;
}

/*!
 Sets the value of the integration time for the GRAYSCALE mode.
 @param us integration time in microseconds
 @param deviceAddress I2C address of the chip (default 0x20)
 @returns On success, 0, on error -1
 @note
 usage with server:
 - set integration time for GRAYSCALE mode to 280us -> setIntegrationTime2D 280
 .
 default
 - setIntegrationTime2D 5000
 */
int16_t apiSetIntegrationTime2D(const int us, const int deviceAddress) {
	return configSetIntegrationTime2D(us, deviceAddress);
}

/*!
 Sets the value of the integration time for the THREED mode.
 @param us integration time in microseconds
 @param deviceAddress address of the I2C device
 @returns On success, 0, on error -1
 @note
 usage with server:
 - set integration time for THREED mode to 120us -> setIntegrationTime3D 120
 .
 default
 - setIntegrationTime3D 700
 */
int16_t apiSetIntegrationTime3D(const int us, const int deviceAddress) {
	return configSetIntegrationTime3D(us, deviceAddress);
}

/*!
 Sets the value of the integration time for the HDR mode.
 @param us integration time in microseconds
 @param deviceAddress address of the I2C device
 @returns On success, 0, on error -1
 @note
 usage with server:
 - set integration time for HDR mode to 120us -> setIntegrationTime3DHDR 120
 .
 default
 - setIntegrationTime3DHDR 700
 */
int16_t apiSetIntegrationTime3DHDR(const int us, const int deviceAddress) {
	return configSetIntegrationTime3DHDR(us, deviceAddress);
}


/*!
 Sets the value of the imaging time.
 @returns imagingTime
 @note
 usage with server:
 -measure imaging time on runtime
 */

int16_t apiGetImagingTime() {
	return configGetImagingTime();
}


/*!
 Enable and disable of adding DCS0+DCS2 and DCS1+DCS3 and check
 with the limit of a threshold.
 @returns enable or disable argument
 @note
 */


int apiEnableAddArgThreshold(const int enableAddArg){
	return configEnableAddArg(enableAddArg);
}




/*!
 Sets threshold value for the sum of the argument DCS0+DCS2 and DCS1+DCS3
 @returns threshold
 @note
 usage with server:
 -set ADCOUV if this threshold is exceeded
 */


int apiSetAddArgThreshold(const int threshold){
	return configSetDcsAdd_threshold(threshold);
}

/*!
 Sets the minimum threshold value for the sum of the argument DCS0+DCS2 and DCS1+DCS3.
 This should be set on a low ToF amplitude (f.egg. 100LSB)
 @returns setArgMin
 @note
 usage with server:
 -set ADCOUV if this threshold is exceeded
 */

int apiSetAddArgMin(const int setArgMin){
	return configSetAddArgMin(setArgMin);
}

/*!
 Sets the maximum threshold value for the sum of the argument DCS0+DCS2 and DCS1+DCS3.
 This should be set on a low ToF amplitude (f.egg. 100LSB)
 @returns threshold
 @note
 usage with server:
 -set ADCOUV if this threshold is exceeded
 */

int apiSetAddArgMax(const int setArgMax){
	return configSetAddArgMax(setArgMax);
}

/*!
 Sets the minimum amplitude for the calculation of the Distance (distance will be set to >30000 if min amplitude is higher than measured value)
=======
 Sets the minimum amplitude for the calculation of the Distance (distance will be set to >LOW_AMPLITUDE if min amplitude is higher than measured value)
>>>>>>> .r13927
 @param minAmplitude minimum value of the amplitude
 @returns value of the minimum Amplitude
 @note
 usage with server:
 - set min amplitude to 150 -> setMinAmplitude 150
 - show all distance values -> setMinAmplitude 0
 .
 default:
 - setMinAmplitude 0
 */
int16_t apiSetMinAmplitude(const int minAmplitude) {
	pruSetMinAmplitude(minAmplitude);
	hysteresisUpdateThresholds(minAmplitude);
	return minAmplitude;
}

/*!
 Gets the minimum amplitude.
 @returns minAmplitude minimum value of the amplitude
 @note
 usage with server:
 - getMinAmplitude
 */
int16_t apiGetMinAmplitude() {
	return pruGetMinAmplitude();
}

/*!
 Sets the modulation frequency.
 @param index modulation frequency index
 @param deviceAddress I2C address of the chip (default 0x20)
 @returns On success, 0, on error -1
 @note
 usage with server:
 - set the modulation frequency to 625kHz -> setModulationFrequency 5
 .....
 - set the modulation frequency to 10MHz -> setModulationFrequency 1
 - set the modulation frequency to 20MHz -> setModulationFrequency 0
 .
 default:
 - setModulationFrequency 1
 */
int16_t apiSetModulationFrequency(const int index, const int deviceAddress) {
	if (index >= 0 && index < configGetModFreqCount()) {
		mod_freq_t freq = (mod_freq_t) index;
		configSetModulationFrequency(freq, deviceAddress);
		imageDifferenceInit();
		calibrationUpdateFrequency();
		return 0;
	}
	return -1;
}

/*!
 The modulation frequencies depend on the clks. Getter for the modulation frequencies.
 @returns the frequencies in khz
 @note
 usage with server:
 - getModulationFrequencies
 */
int* apiGetModulationFrequencies(){
	return configGetModulationFrequencies();
}

/*!
 The calibration type shows, which calibration was found for the correlated modulation frequendy
 @returns the calibration_t type
 @note
 usage with server:
 - GetModulationFrequencyCalibration
 */
int16_t apiGetModulationFrequencyCalibration(int freq){
	int16_t mode;

	mode = getModulationFrequencyCalibration(freq);

	return mode;
}

/*!
 Enables or disables Pi-Delay matching.
 @param state 1 for enable, 0 for disable
 @param deviceAddress I2C address of the chip (default 0x20)
 @returns state of Pi delay
 @note
 usage with server:
 - disable Pi delay matching -> enablePiDelay 0
 .
 default:
 - enablePiDelay 1
 */
int16_t apiEnablePiDelay(const int state, const int deviceAddress) {
	int enable = 1;
	if (!state) {
		enable = 0;
		if (calculationIsDualMGX()) {
			nDCSTOF = 1;
		} else {
			nDCSTOF = 2;
		}
	} else {
		if (calculationIsDualMGX()) {
			nDCSTOF = 2;
		} else {
			nDCSTOF = 4;
		}
	}
	configSetDCS(nDCSTOF, deviceAddress);
	pruSetDCS(nDCSTOF);
	calculationEnablePiDelay(enable);
	imageDifferenceInit();
	return enable;
}

/*!
 Enables or disables Dual MGX Motion blur.
 @param state 1 for enable, 0 for disable
 @param deviceAddress I2C address of the chip (default 0x20)
 @returns state of dual MGX
 @note
 usage with server:
 - enable dual MGX -> enableDualMGX 1
 .
 default:
 - enableDualMGX 0
 */
int16_t apiEnableDualMGX(const int state, const int deviceAddress) {
	int enable = 1;
	if (!state) {
		enable = 0;
		if (calculationIsPiDelay()) {	// no dual mgx, pi delay
			nDCSTOF = 4;
		} else {	// no dual mgx, no pi delay
			nDCSTOF = 2;
		}
	} else {
		if (calculationIsPiDelay()) { // dual mgx, pi delay
			nDCSTOF = 2;
		} else {
			nDCSTOF = 1; // dual mgx, no pi delay
		}
	}
	readOutEnableDualMGX(enable, deviceAddress);
	configSetIntegrationTime3D(configGetIntegrationTime3D(), deviceAddress);
	calculationEnableDualMGX(enable);
	configSetDCS(nDCSTOF, deviceAddress);
	pruSetDCS(nDCSTOF);
	imageCorrectionTransform();
	imageDifferenceInit();
	return enable;
}

/*!
 Enables or disables Dual MGX HDR.
 @param state 1 for enable, 0 for disable
 @param deviceAddress I2C address of the chip (default 0x20)
 @returns state of dual HDR
 @note
 usage with server:
 - enable HDR -> enableHDR 1
 .
 default:
 - enableDualMGX 0
 */
int16_t apiEnableHDR(const int state, const int deviceAddress) {
	int enable = 1;
	if (!state) {
		enable = 0;
	}
	readOutEnableHDR(enable, deviceAddress);
	calculationEnableHDR(enable);
	imageCorrectionTransform();
	configSetIntegrationTimesHDR();
	imageDifferenceInit();
	return enable;
}


/*!
 Sets the offset in cm
 @param offset in cm
 @param deviceAddress I2C address of the chip (default 0x20)
 @returns offset in cm
 @note
 usage with server
 - set Offset to -12cm -> setOffset -12
 .
 default:
 - value from the offset file inside /calibration/offset_user_20.bin or /calibration/offset_factory_20.bin
 */
int16_t apiSetOffset(const int offset, const int deviceAddress) {
	return calibrationSetOffsetCM(offset, deviceAddress);
}

/*!
 Gets the offset.
 @param deviceAddress I2C address of the chip (default 0x20)
 @returns offset in cm
 @note
 usage with server
 - get Offset -> getOffset
 */
int16_t apiGetOffset(const int deviceAddress) {
	return calibrationGetOffsetDefaultCM(deviceAddress);
}

/*!
 Enables or disables the default offset.
 @param state if 0 the Offset set over apiSetOffset will be used else the offset from the calibration
 @returns state
 usage with server
 - take manual offset -> enableDefaultOffset 0
 .
 default:
 - enableDefaultOffset 1
 */
int16_t apiEnableDefaultOffset(const int state) {
	return calibrationEnableDefaultOffset(state);
}

/*!
 Sets the hysteresis for the amplitude (upper threshold will be minAmplitude + hysteresis and the lower threshold will be minAmplitude)
 @returns the value of the hysteresis
 @note
 usage with server
 - set Hysteresis to 20 -> setHysteresis 20
 .
 default:
 - setHysteresis 0
 */
int16_t apiSetHysteresis(const int value) {
	return hysteresisSetThresholds(pruGetMinAmplitude(), value);
}

/*!
 Get coordinates x,y of bad pixels.
 "searchBadPixels" has to be done first. (reads from file)
 @params temps pointer to the array badPixel coordinates.
 @returns the number of bad pixels
 @note usage with server:
 - getBadPixels
 */
int16_t apiGetBadPixels(int16_t *badPixels){
	return calibrationGetBadPixels(badPixels);
}

/*!
 Reads the temperature values out of the ROI (dummy rows) and the illumination board (ADC BBB). The values will be �C x 10 ( 254 -> 25.4�C).
 @param deviceAddress I2C address of the chip (default 0x20)
 @params temps pointer to the array of temperatures:
 [0] -> temp top left (chip)
 [1] -> temp top right (chip)
 [2] -> temp bottom left (chip)
 [3] -> temp bottom right (chip)
 [4] -> temp ADC4 (LED top)
 [5] -> temp ADC2 (LED bottom)
 @returns the number of stored temperatures
 @note usage with server:
 - getTemperature
 @warning offset / slope could be different per chip / has to be evaluated. ADC on Beaglebone shows quite unstable results at the moment.
 */
int16_t apiGetTemperature(unsigned int deviceAddress, int16_t *temps) {
	return temperatureGetTemperatures(deviceAddress, temps);
}

/*!
 Reads the temperature values out of the ROI (dummy rows).
 @param deviceAddress I2C address of the chip (default 0x20)
 @returns the averaged temperature (tr, tl, br, bl). The value will be �C x 10 (421 -> 42.1�C)
 @note usage with server:
 - getAveragedTemperature
 @warning offset / slope could be different per chip / has to be evaluated.
 */
int16_t apiGetAveragedTemperature(unsigned int deviceAddress) {
	return 10 * temperatureGetAveragedChipTemperature(deviceAddress);
}

/*!
 Gets the chip information (IDs, types, versions)
 @param deviceAddress
 @params chipInfo pointer to the array of IDs:
 [0] -> IC_TYPE
 [1] -> IC_VERSION
 [2] -> ENGINEERING_ID
 [3] -> WAFER_ID
 [4] -> CHIP_ID
 [5] -> PART_TYPE
 [6] -> PART_VERSION
 @returns the number of stored IDs
 @note usage with server:
 - getChipInfo
 */
int16_t apiGetChipInfo(const int deviceAddress, int16_t *chipInfo) {
	return chipInfoGetInfo(deviceAddress, chipInfo);
}

/*!
 Selects the modulation mode.
 Select 0 for sinusoidal, 1 for PN
 @param deviceAddress I2C address of the chip (default 0x20)
 @returns selected mode
 @note usage with server:
 - select PN -> selectMode 1
 .
 default:
 - selectMode 0
 */
int16_t apiSelectMode(int select, const int deviceAddress) {
	if (select < 0) {
		select = 0;
	} else if (select > 1) {
		select = 1;
	}
	configSetMode(select, deviceAddress);
	calculationEnablePN(select);
	imageDifferenceInit();
	return select;
}

/*!
 Selects the polynomial with the given index. Each camera has to use an unique PN polynomial
 @params indexLSFR number of LSFR stages -> 1 = 14, 2 = 13 .... 7 = 8
 @param indexPolynomial index of the polynomial for the given LSFR stages (only 5 stored at the moment / 0-4)
 @param deviceAddress I2C address of the chip (default 0x20)
 @returns selected polynomial
 @note usage with server:
 - selectPolynomial 3 3
 */
int16_t apiSelectPolynomial(unsigned int indexLFSR, unsigned int indexPolynomial, const int deviceAddress) {
	return pnSelectPolynomial(indexLFSR, indexPolynomial, deviceAddress);
}

/*!
 Replaces the value of a pixel with the value of a neighbour
 @returns state
 @note usage with server:
 - enable enableImageCorrection 1
 .
 default:
 - enableImageCorrection 0
 @warning
 - experimental
 */
int16_t apiEnableImageCorrection(const unsigned int state) {
	return imageCorrectionEnable(state);
}

/*!
 Averages images over time
 @param number of pictures you want to average over
 @returns number of pictures
 @note usage with server:
 - average over 10 pictures -> setImageAveraging 10
 .
 default:
 - setImageAveraging 10
 */
int16_t apiSetImageAveraging(const unsigned int averageOver) {
	return imageSetNumberOfPicture(averageOver);
}

/*!
 @warning
 - experimental
 */
int16_t apiSetImageProcessing(const unsigned int mode) {
	return imageSetProcessing(mode);
}

/*!
 @warning
 - experimental
 */
int16_t apiSetImageDifferenceThreshold(const unsigned int cm){
	return imageDifferenceSetThreshold(cm);
}

/*!
 Gets the IC_VERSION of the chip
 @returns IC_VERSION
 @note usage with server:
 - getIcVersion
 */
int16_t apiGetIcVersion(){
	return configGetIcVersion();
}

int16_t apiGetPartVersion(){
	return configGetPartVersion();
}


/*!
Version of the Server
 @returns server version
 @note
 usage with server:
 - version
 */
int16_t apiGetVersion(){
	return getVersion();
}

/*!
 Enables or disables saturation
 @param state if not 0, saturation information will be used and saturated pixels will have the value 31000, else the saturation information doesn't matter
 @returns state
 @note usage with server:
 - enableSaturation 1
 .
 default:
 - enableSaturation 0
 */
int16_t apiEnableSaturation(int state){
	return saturationEnable(state);
}

/*!
 Enables or disables adc overflow detection
 @param state if not 0, adc overflow detection information will be used and detected pixels will have the value 32000, else the adc overflow information doesn't matter
 @returns state
 @note usage with server:
 - enableAdcOverflow 1
 .
 default:
 - enableAdcOverflow 0
 */
int16_t apiEnableAdcOverflow(int state){
	return adcOverflowEnable(state);
}

/*!
 Check if FLIM is available
 @returns state
 @note usage with server:
 - isFLIM
 */
int16_t apiIsFLIM() {
	return configIsFLIM();
}

/*!
 Set t1 for FLIM
 @returns register value
 @note usage with server:
  - set t1 to 12.5ns - FLIMSetT1 12.5
  .
 default:
 - FLIMSetT1 25.0
 */
int16_t apiFLIMSetT1(uint16_t value) {
	return flimSetT1(value);
}

/*!
 Set t2 for FLIM
 @returns register value
 @note usage with server:
  - set t2 to 12.5ns - FLIMSetT2 12.5
    .
 default:
 - FLIMSetT2 12.5
 */
int16_t apiFLIMSetT2(uint16_t value) {
	return flimSetT2(value);
}

/*!
 Set t3 for FLIM
 @returns register value
 @note usage with server:
 - set t3 to 12.5ns - FLIMSetT3 12.5
   .
 default:
 - FLIMSetT3 12.5
 */
int16_t apiFLIMSetT3(uint16_t value) {
	return flimSetT3(value);
}

/*!
 Set t4 for FLIM
 @returns register value
 @note usage with server:
  - set t4 to 12.5ns - FLIMSetT4 12.5
    .
 default:
 - FLIMSetT4 50.0
 */
int16_t apiFLIMSetT4(uint16_t value) {
	return flimSetT4(value);
}

/*!
 Set tRep for FLIM
 @returns register value
 @note usage with server:
  - set tRep to 12.5ns - FLIMSetTREP 12.5
    .
 default:
 - FLIMSetTREP 125
 */
int16_t apiFLIMSetTREP(uint16_t value) {
	return flimSetTREP(value);
}

/*!
 Set repetitions for the FLIM sequence
 @returns register value
 @note usage with server:
  - set repetitions to 100 - FLIMSetRepetitions 100
    .
 default:
 - FLIMSetRepetitions 1000
 */
int16_t apiFLIMSetRepetitions(uint16_t value) {
	return flimSetRepetitions(value);
}

/*!
 Set tFlashDelay for FLIM
 @returns register value
 @note usage with server:
   - set tFlashDelay to 12.5ns - FLIMSetFlashDelay 12.5
     .
 default:
 - FLIMSetFlashDelay 12.5
 */
int16_t apiFLIMSetFlashDelay(uint16_t value) {
	return flimSetFlashDelay(value);
}

/*!
 Set tFlashWidth for FLIM
 @returns register value
 @note usage with server:
 - set tFlashWidth to 12.5ns - FLIMSetFlashWidth 12.5
   .
 default:
 - FLIMSetFlashWidth 12.5
 */
int16_t apiFLIMSetFlashWidth(uint16_t value) {
	return flimSetFlashWidth(value);
}

/*!
  Get the FLIM step in ps. (depends on clks)
  @returns FLIM step in ps.
  @note usage with server:
  - FLIMGetStep
 */
int16_t apiFLIMGetStep(){
	return flimGetStep();
}

/*!
 see unsorted
 sorted -> 0/0 first 327/251 last
 */
int apiGetBWSorted(uint16_t **data) {
	return calculationBWSorted(data);
}

/*!
 see unsorted
 sorted -> 0/0 first 327/251 last
 */
int apiGetDCSSorted(uint16_t **data) {
	return calculationDCSSorted(data);
}

int apiGetDCSTOFeAndGrayscaleSorted(uint16_t **data) {
	return getDCSTOFeAndGrayscaleSorted(data, nDCSTOF);	//logs with number of DCS set over mode (pi delay/no pi delay)
}


/*!
 see unsorted
 sorted -> 0/0 first 327/251 last
 */
int apiGetDistanceSorted(uint16_t **data) {

	int16_t mode=0;

	mode = getModulationFrequencyCalibration(configGetModFreqMode());

	if(mode>0){
		//nothing happends
	}else{
		apiCorrectTemperature(false);
		apiCorrectAmbientLight(false);
		apiCorrectDRNU(0);
	}


	return calculationDistanceSorted(data);
}

/*!
 see unsorted
 sorted -> 0/0 first 327/251 last
 */
int apiGetAmplitudeSorted(uint16_t **data) {
	return calculationAmplitudeSorted(data);
}

/*!
 see unsorted
 sorted -> 0/0 first 327/251 last
 */
int apiGetDistanceAndAmplitudeSorted(uint16_t **data) {
	/*double elapsedTime;
	struct timeval tv1, tv2;
	gettimeofday(&tv1, NULL);

	int res = calculationDistanceAndAmplitudeSorted(data);

	gettimeofday(&tv2, NULL);
	elapsedTime = (double)(tv2.tv_sec - tv1.tv_sec) + (double)(tv2.tv_usec - tv1.tv_usec)/1000000.0;
	printf("calculationDistanceAndAmplitudeSorted in seconds: = %fs\n", elapsedTime);
	return res;*/
	int16_t mode=0;

	mode = getModulationFrequencyCalibration(configGetModFreqMode());

	if(mode>0){
		//nothing happend
	}else{
		apiCorrectTemperature(false);
		apiCorrectAmbientLight(false);
		apiCorrectDRNU(0);
	}

	return calculationDistanceAndAmplitudeSorted(data);
}

int apiCalibrateGrayscale(int mode){
	return calculationCalibrateGrayscale(mode);
}

int apiCorrectGrayscaleOffset(int enable){
	setCorrectGrayscaleOffset(enable);
	return 0;
}

int apiCorrectGrayscaleGain(int enable){
	setCorrectGrayscaleGain(enable);
	return 0;
}

int apiCalibrateDRNU(){
	return calibrationDRNU();
}

int apiCorrectDRNU(int enable){

	int16_t mode=0;

	mode = getModulationFrequencyCalibration(configGetModFreqMode());

	if(mode>0){
		calculationSetEnableDRNU(enable);
	}else{
		calculationSetEnableDRNU(false);
	}


	return 0;
}

int apiCorrectTemperature(int enable){
	calculationSetEnableTemperature(enable);
	return 0;
}

int apiCorrectAmbientLight(int enable){
	calculationSetEnableAmbientLight(enable);
	return 0;
}

int16_t apiIsEnabledDRNUCorrection(){
	return configIsEnabledDRNUCorrection();
}

int16_t apiIsEnabledGrayscaleCorrection(){
	return configIsEnabledGrayscaleCorrection();
}

int16_t apiIsEnabledTemperatureCorrection(){
	return configIsEnabledTemperatureCorrection();
}

double apiGetChipTempSimpleKalmanK(){
	return getChipTempSimpleKalmanK();
}

double apiSetChipTempSimpleKalmanK(double K){
	return setChipTempSimpleKalmanK(K);
}

int16_t apiIsEnabledAmbientLightCorrection(){
	return configIsEnabledAmbientLightCorrection();
}

int16_t* apiRenewDRNU(int stepMM){
	return calibrateRenewDRNU(stepMM);
}

int16_t* apiShowDRNU(){
	return calibrateShowDRNU();
}

int16_t* apiLoadTemperatureDRNU(){
	return calibrationLoadTemperatureDRNU();
}

int apiSetDRNUDelay(int delay){
	calibrateSetDRNUCalibrateDelay(delay * 1000); //convert mS -> uS
	configSaveParameters();
	return 0;
}

int apiSetDRNUDiffTemp(double diffTemp){
	calibrateSetDRNUDiffTemp((int)(diffTemp * 10));
	configSaveParameters();
	return 0;
}

int apiSetDRNUAverage(int numAveragedFrames){
	calibrateSetDRNUAverage(numAveragedFrames);
	configSaveParameters();
	return numAveragedFrames;
}


int apiSetpreHeatTemp(int preheatTemperature){
	calibrateSetpreHeatTemp(preheatTemperature);
	return preheatTemperature;
}

int apiPrint(int val){
	int res = configSetPrint(val);
	configSaveParameters();
	return res;
}

int apiSetAmbientLightFactor(int factor){
	calculationSetAmbientLightFactorOrg(factor);
	configSaveParameters();
	double newFactor = factor / sqrt( configGetIntegrationTime3D() );
	return calculationSetAmbientLightFactor(newFactor);
}

int apiEnableKalman(int enableKalman){
	calculationEnableKalman(enableKalman);
	return 0;
}

int apiSetKalmanKdiff(double koef){
	calculationSetKalmanKdiff(koef);
	return 0;
}

int apiSetKalmanK(double koef){
	calculationSetKalmanK(koef);
	return 0;
}

int apiSetKalmanQ(double koef){
	calculationSetKalmanQ(koef);
	return 0;
}

int apiSetKalmanThreshold(int koef){
	calculationSetKalmanThreshold(1, koef);
	return 0;
}


int apiSetKalmanNumCheck(int num){
	calculationSetKalmanNumCheck(num);
	return 0;
}

int apiSetKalmanThreshold2(int koef){
	calculationSetKalmanThreshold(2, koef);
	return 0;
}

int apiSetTempCoef(double koef){
	calibrateSetTempCoef(koef, configGetPartType());
	configSaveParameters();
	return 0;
}

int apiSetSpeedOfLight(double value){
	printf("apiSetSpeedOflight: %2.2f\n", value);
	calibrateSetSpeedOfLightDiv2(value/2);
	configSaveParameters();
	return 0;

}

double apiGetSpeedOfLightDev2(){
	return calibrateGetSpeedOfLightDiv2();
}

int16_t apiCorrectFlimOffset(int enable){
	print(true, 1, "correctFlimOffset %d\n", enable);
	calculationCorrectFlimOffset(enable);
	return 0;
}

int16_t apiCorrectFlimGain(int enable){
	print(true, 1, "correctFlimGain %d\n", enable);
	calculationCorrectFlimGain(enable);
	return 0;
}

int16_t apiCalibrateFlim(char* gates){
	return calculationFlimCalibration(gates);
}

int16_t apiTOF(){
	return configRestartTOF();
}

int16_t apiFLIM(){
	return configRestartFLIM();
}

int16_t apiIsFlimCorrectionAvailable(){
	return calculateIsFlimCorrectionAvailable();
}


int16_t apiSetExtClkGen(int clk){
	return initClkGen(clk);
}

int16_t apiTest(int val){

	printf("test... \n");

	return val;
}

/// @}

