#include "calc_mgx_pi_delay.h"
#include "calculation.h"
#include "pru.h"
#include "fp_atan2.h"
#include "calibration.h"
#include "hysteresis.h"
#include "calculator.h"
#include <math.h>

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

int calcMGXPiDelayGetDist(uint16_t **data) {
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
				pixelDCS0 = pMem[r * nCols * nHalves + c];
				pixelDCS1 = pMem[r * nCols * nHalves + nCols + c];
				pixelDCS2 = pMem[r * nCols * nHalves + c + nPixelPerDCS];
				pixelDCS3 = pMem[r * nCols * nHalves + nCols + c + nPixelPerDCS];
				arg2 = (pixelDCS2 & pixelMask) - (pixelDCS0 & pixelMask);
				arg1 = (pixelDCS3 & pixelMask) - (pixelDCS1 & pixelMask);
				if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
						arg1 = arg1 >> 4;
						arg2 = arg2 >> 4;
				}
				amplitude = sqrt((arg1 * arg1) + (arg2 * arg2));
				if (hysteresisUpdate(r * nCols + c, amplitude)) {
					distance = fp_atan2(arg1, arg2);
					distance = ((distance * MAX_DIST_VALUE) / FP_M_2_PI) + offset + calibrationMap[r * nCols + c] + 0.5;
					distance = (distance + MODULO_SHIFT) % MAX_DIST_VALUE;
					pixelData[(r*nHalves/2) * nCols + c] = (int16_t) distance;
				} else {
					pixelData[(r*nHalves/2) * nCols + c] = LOW_AMPLITUDE;
				}


				if (!(configGetPartType() == EPC635 || configGetPartType() == EPC503)) {
					//bottom pixelfield row (arg1 and arg2 are swapped due to swapped DCS0 and DCS1)
					pixelDCS0 = pMem[r * nCols * nHalves + 2 * nCols + c];
					pixelDCS1 = pMem[r * nCols * nHalves + 3 * nCols + c];
					pixelDCS2 = pMem[r * nCols * nHalves + 2 * nCols + c + nPixelPerDCS];
					pixelDCS3 = pMem[r * nCols * nHalves + 3 * nCols + c + nPixelPerDCS];
					arg1 = (pixelDCS3 & pixelMask) - (pixelDCS1 & pixelMask);
					arg2 = (pixelDCS2 & pixelMask) - (pixelDCS0 & pixelMask);
					amplitude = sqrt((arg1 * arg1) + (arg2 * arg2));
					if (hysteresisUpdate(r * nCols + nCols + c, amplitude)) {
						distance = fp_atan2(arg1, arg2);
						distance = ((distance * MAX_DIST_VALUE) / FP_M_2_PI) + offset + calibrationMap[r * nCols + nCols + c] + 0.5;
						distance = (distance + MODULO_SHIFT) % MAX_DIST_VALUE;
					} else {
						distance = LOW_AMPLITUDE;
					}
					pixelData[r * nCols + nCols + c] = (int16_t)distance;
				}

			}
		}
	} else {
		for (r = 0; r < nRowsPerHalf - 1; r += 2) {
			for (c = 0; c < nCols; c++) {
				//top pixelfield row
				pixelDCS0 = pMem[r * nCols * nHalves + c];
				pixelDCS1 = pMem[r * nCols * nHalves + nCols + c];
				pixelDCS2 = pMem[r * nCols * nHalves + c + nPixelPerDCS];
				pixelDCS3 = pMem[r * nCols * nHalves + nCols + c + nPixelPerDCS];
				arg2 = (pixelDCS2 & pixelMask) - (pixelDCS0 & pixelMask);
				arg1 = (pixelDCS3 & pixelMask) - (pixelDCS1 & pixelMask);

				if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
						arg1 = arg1 >> 4;
						arg2 = arg2 >> 4;
				}

				distance = fp_atan2(arg1, arg2);
				distance = ((distance * MAX_DIST_VALUE) / FP_M_2_PI) + offset + calibrationMap[r * nCols + c] + 0.5;
				distance = (distance + MODULO_SHIFT) % MAX_DIST_VALUE;
				pixelData[(r*nHalves/2) * nCols + c] = (int16_t) distance;

				if(!(configGetPartType() == EPC635 || configGetPartType() == EPC503)) {
					//bottom pixelfield row (arg1 and arg2 are swapped due to swapped DCS0 and DCS1)
					pixelDCS0 = pMem[r * nCols * nHalves + 2 * nCols + c];
					pixelDCS1 = pMem[r * nCols * nHalves + 3 * nCols + c];
					pixelDCS2 = pMem[r * nCols * nHalves + 2 * nCols + c + nPixelPerDCS];
					pixelDCS3 = pMem[r * nCols * nHalves + 3 * nCols + c + nPixelPerDCS];
					arg1 = (pixelDCS3 & pixelMask) - (pixelDCS1 & pixelMask);
					arg2 = (pixelDCS2 & pixelMask) - (pixelDCS0 & pixelMask);
					distance = fp_atan2(arg1, arg2);
					distance = ((distance * MAX_DIST_VALUE) / FP_M_2_PI) + offset + calibrationMap[r * nCols + nCols + c] + 0.5;
					distance = (distance + MODULO_SHIFT) % MAX_DIST_VALUE;
					pixelData[r * nCols + nCols + c] = (int16_t) distance;
				}

			}
		}
	}
	*data = pixelData;
	return size / 4;
}

int calcMGXPiDelayGetAmp(uint16_t **data) {
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
			arg2 = (pMem[r * nCols * nHalves + c + nPixelPerDCS] & pixelMask) - (pMem[r * nCols * nHalves + c] & pixelMask);
			arg1 = (pMem[r * nCols * nHalves + nCols + c + nPixelPerDCS] & pixelMask) - (pMem[r * nCols * nHalves + nCols + c] & pixelMask);

			if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
					arg1 = arg1 >> 4;
					arg2 = arg2 >> 4;
			}

			result = sqrt((arg1 * arg1 / 4) + (arg2 * arg2 / 4));
			pixelData[(r*nHalves/2) * nCols + c] = result;

			if (!(configGetPartType() == EPC635 || configGetPartType() == EPC503)) {
				//bottom pixelfield row (arg1 and arg2 are swapped due to swapped DCS0 and DCS1)
				arg2 = (pMem[r * nCols * nHalves + 2 * nCols + c + nPixelPerDCS] & pixelMask) - (pMem[r * nCols * nHalves + 2 * nCols + c] & pixelMask);
				arg1 = (pMem[r * nCols * nHalves + 3 * nCols + c + nPixelPerDCS] & pixelMask) - (pMem[r * nCols * nHalves + 3 * nCols + c] & pixelMask);
				result = sqrt((arg1 * arg1 / 4) + (arg2 * arg2 / 4));
				pixelData[r * nCols + nCols + c] = result;
			}
		}
	}
	*data = pixelData;
	return size / 4;
}

int calcMGXPiDelayGetInfo(uint16_t **data) {
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
			pixelDCS0 = pMem[r * nCols * nHalves + c];
			pixelDCS1 = pMem[r * nCols * nHalves + nCols + c];
			pixelDCS2 = pMem[r * nCols * nHalves + c + nPixelPerDCS];
			pixelDCS3 = pMem[r * nCols * nHalves + nCols + c + nPixelPerDCS];
			arg2 = (pixelDCS2 & pixelMask) - (pixelDCS0 & pixelMask);
			arg1 = (pixelDCS3 & pixelMask) - (pixelDCS1 & pixelMask);

			if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
					arg1 = arg1 >> 4;
					arg2 = arg2 >> 4;
			}

			amplitude = sqrt((arg1 * arg1 / 4) + (arg2 * arg2 / 4));
			if (hysteresisUpdate(r * nCols + c, amplitude)) {
				distance = fp_atan2(arg1, arg2);
				distance = ((distance * MAX_DIST_VALUE) / FP_M_2_PI) + offset + calibrationMap[r * nCols + c] + 0.5;
				distance = (distance + MODULO_SHIFT) % MAX_DIST_VALUE;
				pixelData[(r*nHalves/2) * nCols + c] = (int16_t) distance;
			} else {
				pixelData[(r*nHalves/2) * nCols + c] = LOW_AMPLITUDE;
			}
			pixelData[(r*nHalves/2) * nCols + c + nPixelPerDCS / 2] = amplitude;

			if(!(configGetPartType() == EPC635 || configGetPartType() == EPC503)) {
				//bottom pixelfield row (arg1 and arg2 are swapped due to swapped DCS0 and DCS1)
				pixelDCS0 = pMem[r * nCols * nHalves + 2 * nCols + c];
				pixelDCS1 = pMem[r * nCols * nHalves + 3 * nCols + c];
				pixelDCS2 = pMem[r * nCols * nHalves + 2 * nCols + c + nPixelPerDCS];
				pixelDCS3 = pMem[r * nCols * nHalves + 3 * nCols + c + nPixelPerDCS];
				arg1 = (pixelDCS3 & pixelMask) - (pixelDCS1 & pixelMask);
				arg2 = (pixelDCS2 & pixelMask) - (pixelDCS0 & pixelMask);
				amplitude = sqrt((arg1 * arg1 / 4) + (arg2 * arg2 / 4));
				if (hysteresisUpdate(r * nCols + nCols + c, amplitude)) {
					distance = fp_atan2(arg1, arg2);
					distance = ((distance * MAX_DIST_VALUE) / FP_M_2_PI) + offset + calibrationMap[r * nCols + nCols + c] + 0.5;
				} else {
					amplitude = LOW_AMPLITUDE;
				}
				pixelData[r * nCols + nCols + c] = (int16_t)(distance);
				pixelData[r * nCols + nCols + c + nPixelPerDCS / 2] = amplitude;
			}


		}
	}
	*data = pixelData;
	return size / 2;
}
