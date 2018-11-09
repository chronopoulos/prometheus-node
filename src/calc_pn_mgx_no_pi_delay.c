#include "calc_pn_mgx_no_pi_delay.h"
#include "calculation.h"
#include "pru.h"
#include "fp_atan2.h"
#include "calibration.h"
#include "hysteresis.h"
#include "calculator.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

static uint16_t pixelData[328 * 252 * 2];
int32_t arg1;
int32_t arg2;
int32_t pixelDCS0;
int32_t pixelDCS1;
int16_t amplitude;
int32_t distance;
int i, r, c, result;

int calcPNMGXNoPiDelayGetDist(uint16_t **data) {
	int offset = calibrationGetOffsetPhase();
	int size = pruGetImage(data);
	uint16_t *pMem = *data;
	int nCols = pruGetNCols();
	int nRowsPerHalf = pruGetNRowsPerHalf();
	uint16_t nHalves = pruGetNumberOfHalves();
	uint16_t pixelMask = calculatorGetPixelMask();
	int minAmplitude = pruGetMinAmplitude();

	if (minAmplitude > 0) {
		for (r = 0; r < nRowsPerHalf - 1; r += 2) {
			for (c = 0; c < nCols; c++) {
				//top pixelfield row
				arg2 =  (pMem[r * nCols * nHalves + c] & pixelMask);
				arg1 =  (pMem[r * nCols * nHalves + nCols + c] & pixelMask);
				if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
					arg1 = arg1 >> 4;
					arg2 = arg2 >> 4;
				}
				arg2 = 2048 - arg2;
				arg1 = 2048 - arg1;
				amplitude = abs(arg2) + abs(arg1); // amplitude divider = 1
				if (hysteresisUpdate(i, amplitude)) {
					distance = (double) abs(arg1) / (abs(arg2) + abs(arg1)) * MAX_DIST_VALUE + offset + 0.5;
				} else {
					distance = LOW_AMPLITUDE;
				}
				pixelData[(r*nHalves/2) * nCols + c] = distance;


				//bottom pixelfield row (arg1 and arg2 are swapped due to swapped DCS0 and DCS1)
				if (!(configGetPartType() == EPC635 || configGetPartType() == EPC503)) {
					arg2 = 2048 - (pMem[r * nCols * nHalves + 2 * nCols + c] & pixelMask);
					arg1 = 2048 - (pMem[r * nCols * nHalves + 3 * nCols + c] & pixelMask);
					amplitude = abs(arg2) + abs(arg1); // amplitude divider = 1
					if (hysteresisUpdate(i, amplitude)) {
						distance = (double) abs(arg1) / (abs(arg2) + abs(arg1)) * MAX_DIST_VALUE + offset + 0.5;
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
				arg2 = (pMem[r * nCols * nHalves + c] & pixelMask);
				arg1 = (pMem[r * nCols * nHalves + nCols + c] & pixelMask);

				if(configGetPartType() == EPC635 || configGetPartType() == EPC503) {
					arg1 = arg1 >> 4;
					arg2 = arg2 >> 4;
				}
				arg2 = 2048 - arg2;
				arg1 = 2048 - arg1;
				distance = (double) abs(arg1) / (abs(arg2) + abs(arg1)) * MAX_DIST_VALUE + offset + 0.5;
				pixelData[(r*nHalves/2) * nCols + c] = distance;

				if (!(configGetPartType() == EPC635 || configGetPartType() == EPC503)) {
					//bottom pixelfield row (arg1 and arg2 are swapped due to swapped DCS0 and DCS1)
					arg2 = 2048 - (pMem[ r * nCols * nHalves + 2 * nCols + c] & pixelMask);
					arg1 = 2048 - (pMem[ r * nCols * nHalves + 3 * nCols + c] & pixelMask);
					distance = (double) abs(arg1) / (abs(arg2) + abs(arg1)) * MAX_DIST_VALUE + offset + 0.5;
					pixelData[r * nCols + nCols + c] = distance;
				}
			}
		}
	}
	*data = pixelData;
	return size / 2;
}

int calcPNMGXNoPiDelayGetAmp(uint16_t **data) {
	int size = pruGetImage(data);
	uint16_t *pMem = *data;
	int nCols = pruGetNCols();
	int nRowsPerHalf = pruGetNRowsPerHalf();
	uint16_t nHalves = pruGetNumberOfHalves();
	uint16_t pixelMask = calculatorGetPixelMask();
	for (r = 0; r < nRowsPerHalf - 1; r += 2) {
		for (c = 0; c < nCols; c++) {
			//top pixelfield row
			arg2 = (pMem[r * nCols * nHalves + c] & pixelMask);
			arg1 = (pMem[r * nCols * nHalves + nCols + c] & pixelMask);
			if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
				arg1 = arg1 >> 4;
				arg2 = arg2 >> 4;
			}
			arg2 = 2048 - arg2;
			arg1 = 2048 - arg1;

			result = abs(arg2) + abs(arg1); // amplitude divider = 1
			pixelData[(r*nHalves/2) * nCols + c] = result;

			if (!(configGetPartType() == EPC635 || configGetPartType() == EPC503)) {
				//bottom pixelfield row (arg1 and arg2 are swapped due to swapped DCS0 and DCS1)
				arg2 = 2048 - (pMem[r * nCols * nHalves + 2 * nCols + c] & pixelMask);
				arg1 = 2048 - (pMem[r * nCols * nHalves + 3 * nCols + c] & pixelMask);
				result = abs(arg2) + abs(arg1); // amplitude divider = 1
				pixelData[r * nCols + nCols + c] = result;
			}
		}
	}
	*data = pixelData;
	return size / 2;
}

int calcPNMGXNoPiDelayGetInfo(uint16_t **data) {
	int offset = calibrationGetOffsetPhase();
	int size = pruGetImage(data);
	uint16_t *pMem = *data;
	int nPixelPerDCS = size;
	int nCols = pruGetNCols();
	int nRowsPerHalf = pruGetNRowsPerHalf();
	uint16_t nHalves = pruGetNumberOfHalves();
	uint16_t pixelMask = calculatorGetPixelMask();

	for (r = 0; r < nRowsPerHalf - 1; r += 2) {
		for (c = 0; c < nCols; c++) {
			//top pixelfield row
			arg2 = (pMem[r * nCols * nHalves + c] & pixelMask);
			arg1 = (pMem[r * nCols * nHalves + nCols + c] & pixelMask);
			if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
				arg1 = arg1 >> 4;
				arg2 = arg2 >> 4;
			}
			arg2 = 2048 - arg2;
			arg1 = 2048 - arg1;

			amplitude = abs(arg2) + abs(arg1); // amplitude divider = 1
			if (hysteresisUpdate(i, amplitude)){
				distance = (double) abs(arg1) / (abs(arg2) + abs(arg1)) * MAX_DIST_VALUE + offset + 0.5;
			}
			else{
				distance = LOW_AMPLITUDE;
			}
			pixelData[(r*nHalves/2) * nCols + c] = distance;
			pixelData[(r*nHalves/2) * nCols + c + nPixelPerDCS / 2] = amplitude;

			if (!(configGetPartType() == EPC635 || configGetPartType() == EPC503)) {
				//bottom pixelfield row (arg1 and arg2 are swapped due to swapped DCS0 and DCS1)
				arg2 = 2048 - (pMem[r * nCols * nHalves + 2 * nCols + c] & pixelMask);
				arg1 = 2048 - (pMem[r * nCols * nHalves + 3 * nCols + c] & pixelMask);
				amplitude = abs(arg2) + abs(arg1); // amplitude divider = 1
				if (hysteresisUpdate(i, amplitude)){
					distance = (double) abs(arg1) / (abs(arg2) + abs(arg1)) * MAX_DIST_VALUE + offset + 0.5;
				}
				else{
					distance = LOW_AMPLITUDE;
				}
				pixelData[r * nCols + nCols + c] = distance;
				pixelData[r * nCols + nCols + c + nPixelPerDCS / 2] = amplitude;
			}
		}
	}
	*data = pixelData;
	return size;
}
