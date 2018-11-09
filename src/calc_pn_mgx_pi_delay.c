#include "calc_pn_mgx_pi_delay.h"
#include "calculation.h"
#include "calculator.h"
#include "pru.h"
#include "fp_atan2.h"
#include "calibration.h"
#include "hysteresis.h"
#include <math.h>
#include <stdlib.h>

static uint16_t pixelData[328 * 252 * 2];
int32_t arg1;
int32_t arg2;
int32_t pixelDCS0;
int32_t pixelDCS1;
int32_t pixelDCS2;
int32_t pixelDCS3;
int16_t amplitude;
int32_t distance;
int i, r, c, result;

int calcPNMGXPiDelayGetDist(uint16_t **data) {
	int offset = calibrationGetOffsetPhase();
	int size = pruGetImage(data);
	uint16_t *pMem = *data;
	int nCols = pruGetNCols();
	int nRowsPerHalf = pruGetNRowsPerHalf();
	int32_t *calibrationMap = calibrationGetCorrectionMap();
	uint16_t pixelMask = calculatorGetPixelMask();
	uint16_t nHalves = pruGetNumberOfHalves();
	int minAmplitude = pruGetMinAmplitude();
	int nPixelPerDCS = size / 2;
	if (minAmplitude > 0) {
		for (r = 0; r < nRowsPerHalf - 1; r += 2) {
			for (c = 0; c < nCols; c++) {
				//top pixelfield row
				pixelDCS0 = pMem[r * nCols * nHalves + c] & pixelMask;
				pixelDCS1 = pMem[r * nCols * nHalves + nCols + c] & pixelMask;
				pixelDCS2 = pMem[r * nCols * nHalves + c + nPixelPerDCS] & pixelMask;
				pixelDCS3 = pMem[r * nCols * nHalves + nCols + c + nPixelPerDCS] & pixelMask;
				arg1 = pixelDCS3 - pixelDCS1;
				arg2 = pixelDCS2 - pixelDCS0;
				if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
					arg1 = arg1 >> 4;
					arg2 = arg2 >> 4;
				}
				amplitude = (abs(arg2) + abs(arg1)) / 2;
				if (hysteresisUpdate(i, amplitude)) {
					distance = ((double) abs(arg1) / (abs(arg2) + abs(arg1))) * MAX_DIST_VALUE + offset + calibrationMap[r * nCols + c] + 0.5;
				} else {
					distance = LOW_AMPLITUDE;
				}
				pixelData[(r*nHalves/2) * nCols + c] = distance;

				if (!(configGetPartType() == EPC635 || configGetPartType() == EPC503)) {
					//bottom pixelfield row (arg1 and arg2 are swapped due to swapped DCS0 and DCS1)
					pixelDCS0 = pMem[r * nCols * nHalves + 2 * nCols + c] & pixelMask;
					pixelDCS1 = pMem[r * nCols * nHalves + 3 * nCols + c] & pixelMask;
					pixelDCS2 = pMem[r * nCols * nHalves + 2 * nCols + c + nPixelPerDCS] & pixelMask;
					pixelDCS3 = pMem[r * nCols * nHalves + 3 * nCols + c + nPixelPerDCS] & pixelMask;
					arg1 = pixelDCS3 - pixelDCS1;
					arg2 = pixelDCS2 - pixelDCS0;
					amplitude = (abs(arg2) + abs(arg1)) / 2;
					if (hysteresisUpdate(i, amplitude)) {
						distance = ((double) abs(arg1) / (abs(arg2) + abs(arg1))) * MAX_DIST_VALUE + offset + calibrationMap[r * nCols + nCols + c] + 0.5;
					} else {
						distance = LOW_AMPLITUDE;
					}
					pixelData[r * nCols + nCols + c] = distance;
				}
			}
		}
	} else {
		for (r = 0; r < nRowsPerHalf - 1; r += 2) {
			for (c = 0; c < nCols; c++) {
				//top pixelfield row
				pixelDCS0 = pMem[r * nCols * nHalves + c] & pixelMask;
				pixelDCS1 = pMem[r * nCols * nHalves + nCols + c] & pixelMask;
				pixelDCS2 = pMem[r * nCols * nHalves + c + nPixelPerDCS] & pixelMask;
				pixelDCS3 = pMem[r * nCols * nHalves + nCols + c + nPixelPerDCS] & pixelMask;
				arg1 = pixelDCS3 - pixelDCS1;
				arg2 = pixelDCS2 - pixelDCS0;
				if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
					arg1 = arg1 >> 4;
					arg2 = arg2 >> 4;
				}
				distance = ((double) abs(arg1) / (abs(arg2) + abs(arg1))) * MAX_DIST_VALUE + offset + calibrationMap[r * nCols + c] + 0.5;
				pixelData[(r*nHalves/2) * nCols + c] = distance;

				if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
					//bottom pixelfield row (arg1 and arg2 are swapped due to swapped DCS0 and DCS1)
					pixelDCS0 = pMem[r * nCols * nHalves + 2 * nCols + c] & pixelMask;
					pixelDCS1 = pMem[r * nCols * nHalves + 3 * nCols + c] & pixelMask;
					pixelDCS2 = pMem[r * nCols * nHalves + 2 * nCols + c + nPixelPerDCS] & pixelMask;
					pixelDCS3 = pMem[r * nCols * nHalves + 3 * nCols + c + nPixelPerDCS] & pixelMask;
					arg1 = pixelDCS3 - pixelDCS1;
					arg2 = pixelDCS2 - pixelDCS0;
					distance = ((double) abs(arg1) / (abs(arg2) + abs(arg1))) * MAX_DIST_VALUE + offset + calibrationMap[r * nCols + nCols + c] + 0.5;
					pixelData[r * nCols + nCols + c] = distance;
				}
			}
		}
	}
	*data = pixelData;
	return size / 4;
}

int calcPNMGXPiDelayGetAmp(uint16_t **data) {
	int size = pruGetImage(data);
	uint16_t *pMem = *data;
	int nPixelPerDCS = size / 2;
	int nCols = pruGetNCols();
	int nRowsPerHalf = pruGetNRowsPerHalf();
	uint16_t nHalves = pruGetNumberOfHalves();
	uint16_t pixelMask = calculatorGetPixelMask();
	for (r = 0; r < nRowsPerHalf - 1; r += 2) {
		for (c = 0; c < nCols; c++) {
			//top pixelfield row
			pixelDCS0 = pMem[r * nCols * nHalves + c] & pixelMask;
			pixelDCS1 = pMem[r * nCols * nHalves + nCols + c] & pixelMask;
			pixelDCS2 = pMem[r * nCols * nHalves + c + nPixelPerDCS] & pixelMask;
			pixelDCS3 = pMem[r * nCols * nHalves + nCols + c + nPixelPerDCS] & pixelMask;
			arg1 = pixelDCS3 - pixelDCS1;
			arg2 = pixelDCS2 - pixelDCS0;
			if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
				arg1 = arg1 >> 4;
				arg2 = arg2 >> 4;
			}
			amplitude = (abs(arg2) + abs(arg1)) / 2;
			pixelData[(r*nHalves/2) * nCols + c] = amplitude;

			if (!(configGetPartType() == EPC635 || configGetPartType() == EPC503)) {
				//bottom pixelfield row (arg1 and arg2 are swapped due to swapped DCS0 and DCS1)
				pixelDCS0 = pMem[r * nCols * nHalves + 2 * nCols + c] & pixelMask;
				pixelDCS1 = pMem[r * nCols * nHalves + 3 * nCols + c] & pixelMask;
				pixelDCS2 = pMem[r * nCols * nHalves + 2 * nCols + c + nPixelPerDCS] & pixelMask;
				pixelDCS3 = pMem[r * nCols * nHalves + 3 * nCols + c + nPixelPerDCS] & pixelMask;
				arg1 = pixelDCS3 - pixelDCS1;
				arg2 = pixelDCS2 - pixelDCS0;
				amplitude = (abs(arg2) + abs(arg1)) / 2;
				pixelData[r * nCols + nCols + c] = amplitude;
			}
		}
	}
	*data = pixelData;
	return size / 4;
}

int calcPNMGXPiDelayGetInfo(uint16_t **data) {
	int offset = calibrationGetOffsetPhase();
	int size = pruGetImage(data);
	uint16_t *pMem = *data;
	int nPixelPerDCS = size / 2;
	int nCols = pruGetNCols();
	int nRowsPerHalf = pruGetNRowsPerHalf();
	uint16_t nHalves = pruGetNumberOfHalves();
	int32_t *calibrationMap = calibrationGetCorrectionMap();
	uint16_t pixelMask = calculatorGetPixelMask();
	for (r = 0; r < nRowsPerHalf - 1; r += 2) {
		for (c = 0; c < nCols; c++) {
			//top pixelfield row
			pixelDCS0 = pMem[r * nCols * nHalves + c] & pixelMask;
			pixelDCS1 = pMem[r * nCols * nHalves + nCols + c] & pixelMask;
			pixelDCS2 = pMem[r * nCols * nHalves + c + nPixelPerDCS] & pixelMask;
			pixelDCS3 = pMem[r * nCols * nHalves + nCols + c + nPixelPerDCS] & pixelMask;
			arg1 = pixelDCS3 - pixelDCS1;
			arg2 = pixelDCS2 - pixelDCS0;
			if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
				arg1 = arg1 >> 4;
				arg2 = arg2 >> 4;
			}
			amplitude = (abs(arg2) + abs(arg1)) / 2;
			if (hysteresisUpdate(i, amplitude)){
				distance = ((double) abs(arg1) / (abs(arg2) + abs(arg1))) * MAX_DIST_VALUE + offset + calibrationMap[r * nCols + c] + 0.5;
			}
			else{
				distance = LOW_AMPLITUDE;
			}
			pixelData[(r*nHalves/2) * nCols + c] = (uint16_t)distance;
			pixelData[(r*nHalves/2) * nCols + c + nPixelPerDCS / 2] = (uint16_t)amplitude;

			if (!(configGetPartType() == EPC635 || configGetPartType() == EPC503)) {
				//bottom pixelfield row (arg1 and arg2 are swapped due to swapped DCS0 and DCS1)
				pixelDCS0 = pMem[r * nCols * nHalves + 2 * nCols + c] & pixelMask;
				pixelDCS1 = pMem[r * nCols * nHalves + 3 * nCols + c] & pixelMask;
				pixelDCS2 = pMem[r * nCols * nHalves + 2 * nCols + c + nPixelPerDCS] & pixelMask;
				pixelDCS3 = pMem[r * nCols * nHalves + 3 * nCols + c + nPixelPerDCS] & pixelMask;
				arg1 = pixelDCS3 - pixelDCS1;
				arg2 = pixelDCS2 - pixelDCS0;
				amplitude = (abs(arg2) + abs(arg1)) / 2;
				if (hysteresisUpdate(i, amplitude)){
					distance = ((double) abs(arg1) / (abs(arg2) + abs(arg1))) * MAX_DIST_VALUE + offset + calibrationMap[r * nCols + nCols + c] + 0.5;
				}
				else{
					distance = LOW_AMPLITUDE;
				}
				pixelData[r * nCols + nCols + c] = (uint16_t)distance;
				pixelData[r * nCols + nCols + c + nPixelPerDCS / 2] = (uint16_t)amplitude;
			}

		}
	}
	*data = pixelData;
	return size / 2;
}
