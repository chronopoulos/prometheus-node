#include "calc_pn_def_pi_delay.h"
#include "calculation.h"
#include "pru.h"
#include "fp_atan2.h"
#include "calibration.h"
#include "hysteresis.h"
#include "calculator.h"
#include <math.h>
#include <stdlib.h>

static uint16_t pixelData[328 * 252 * 2];
int32_t arg1;
int32_t arg2;
uint16_t pixelDCS0;
uint16_t pixelDCS1;
uint16_t pixelDCS2;
uint16_t pixelDCS3;
int16_t amplitude;
int32_t distance;
int i, r, c;

int calcPNDefPiDelayGetDist(uint16_t **data) {
	int offset = calibrationGetOffsetPhase();
	int size = pruGetImage(data);
	uint16_t *pMem = *data;
	int32_t *calibrationMap = calibrationGetCorrectionMap();
	uint16_t pixelMask = calculatorGetPixelMask();
	int minAmplitude = pruGetMinAmplitude();
	int nPixelPerDCS = size / 4;
	if (minAmplitude > 0) {
		for (i = 0; i < nPixelPerDCS; i++) {
			pixelDCS0 = pMem[i] & pixelMask;
			pixelDCS1 = pMem[i + nPixelPerDCS] & pixelMask;
			pixelDCS2 = pMem[i + 2 * nPixelPerDCS] & pixelMask;
			pixelDCS3 = pMem[i + 3 * nPixelPerDCS] & pixelMask;

			arg1 = pixelDCS3 - pixelDCS1;
			arg2 = pixelDCS2 - pixelDCS0;

			if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
				arg1 = arg1 >> 4;
				arg2 = arg2 >> 4;
			}

			amplitude = (abs(arg2) + abs(arg1)) / 2;
			if (hysteresisUpdate(i, amplitude)) {
				pixelData[i] = ((double) abs(arg1) / (abs(arg2) + abs(arg1))) * MAX_DIST_VALUE + offset + calibrationMap[i] + 0.5;
			} else {
				pixelData[i] = LOW_AMPLITUDE;
			}
		}
	} else {
		for (i = 0; i < nPixelPerDCS; i++) {
			pixelDCS0 = pMem[i] & pixelMask;
			pixelDCS1 = pMem[i + nPixelPerDCS] & pixelMask;
			pixelDCS2 = pMem[i + 2 * nPixelPerDCS] & pixelMask;
			pixelDCS3 = pMem[i + 3 * nPixelPerDCS] & pixelMask;

			arg1 = pixelDCS3 - pixelDCS1;
			arg2 = pixelDCS2 - pixelDCS0;

			if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
				arg1 = arg1 >> 4;
				arg2 = arg2 >> 4;
			}

			int32_t distance = ((double) abs(arg1) / (abs(arg2) + abs(arg1))) * MAX_DIST_VALUE + offset + calibrationMap[i] + 0.5;
			pixelData[i] = (uint16_t) distance;
		}
	}
	*data = pixelData;
	return nPixelPerDCS;
}

int calcPNDefPiDelayGetAmp(uint16_t **data) {
	int size = pruGetImage(data);
	uint16_t pixelMask = calculatorGetPixelMask();
	uint16_t *pMem = *data;
	int nPixelPerDCS = size / 4;
	for (i = 0; i < nPixelPerDCS; i++) {
		pixelDCS0 = pMem[i] & pixelMask;
		pixelDCS1 = pMem[i + nPixelPerDCS] & pixelMask;
		pixelDCS2 = pMem[i + 2 * nPixelPerDCS] & pixelMask;
		pixelDCS3 = pMem[i + 3 * nPixelPerDCS] & pixelMask;

		arg1 = pixelDCS3 - pixelDCS1;
		arg2 = pixelDCS2 - pixelDCS0;

		if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
			arg1 = arg1 >> 4;
			arg2 = arg2 >> 4;
		}

		pixelData[i] = (abs(arg2) + abs(arg1))/2;
	}
	*data = pixelData;
	return nPixelPerDCS;
}

int calcPNDefPiDelayGetInfo(uint16_t **data) {
	int offset = calibrationGetOffsetPhase();
	int size = pruGetImage(data);
	uint16_t *pMem = *data;
	int32_t *calibrationMap = calibrationGetCorrectionMap();
	uint16_t pixelMask = calculatorGetPixelMask();
	int nPixelPerDCS = size / 4;
	for (i = 0; i < nPixelPerDCS; i++) {
		pixelDCS0 = pMem[i] & pixelMask;
		pixelDCS1 = pMem[i + nPixelPerDCS] & pixelMask;
		pixelDCS2 = pMem[i + 2 * nPixelPerDCS] & pixelMask;
		pixelDCS3 = pMem[i + 3 * nPixelPerDCS] & pixelMask;

		arg1 = pixelDCS3 - pixelDCS1;
		arg2 = pixelDCS2 - pixelDCS0;

		if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
			arg1 = arg1 >> 4;
			arg2 = arg2 >> 4;
		}

		amplitude = (abs(arg2) + abs(arg1)) / 2;
		if (hysteresisUpdate(i, amplitude)) {
			 int32_t distance = ((double) abs(arg1) / (abs(arg2) + abs(arg1))) * MAX_DIST_VALUE + offset + calibrationMap[i] + 0.5;
			 pixelData[i] = (uint16_t)distance;
		} else {
			pixelData[i] = LOW_AMPLITUDE;
		}
		pixelData[i + nPixelPerDCS] = amplitude;
	}
	*data = pixelData;
	return nPixelPerDCS * 2;
}



