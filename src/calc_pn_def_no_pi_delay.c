#include "calc_pn_def_no_pi_delay.h"
#include "calculation.h"
#include "pru.h"
#include "fp_atan2.h"
#include "calibration.h"
#include "hysteresis.h"
#include "calculator.h"
#include <math.h>
#include <stdlib.h>

#include "evalkit_constants.h" // ckc

static uint16_t pixelData[328 * 252 * 2];
int32_t arg1;
int32_t arg2;
uint16_t pixelDCS0;
uint16_t pixelDCS1;
int16_t amplitude;
int32_t distance;
int i, r, c;

int calcPNDefNoPiDelayGetDist(uint16_t **data) {
	int offset = calibrationGetOffsetPhase();
	int size = pruGetImage(data);
	uint16_t *pMem = *data;
	uint16_t pixelMask = calculatorGetPixelMask();
	int minAmplitude = pruGetMinAmplitude();
	int nPixelPerDCS = size / 2;
	if (minAmplitude > 0) {
		for (i = 0; i < nPixelPerDCS; i++) {
			arg2 = (pMem[i] & pixelMask);
			arg1 = (pMem[i + nPixelPerDCS] & pixelMask);

			if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
				arg1 = arg1 >> 4;
				arg2 = arg2 >> 4;
			}
			arg2 = 2048 - arg2;
			arg1 = 2048 - arg1;

			amplitude = abs(arg2) + abs(arg1); // amplitude divider = 1
			if (hysteresisUpdate(i, amplitude)) {
				pixelData[i] = (double) abs(arg1) / (abs(arg2) + abs(arg1)) * MAX_DIST_VALUE + offset + 0.5;
			} else {
				pixelData[i] = LOW_AMPLITUDE;
			}
		}
	} else {
		int nPixelPerDCS = size / 2;
		for (i = 0; i < nPixelPerDCS; i++) {
			arg2 = (pMem[i] & pixelMask);
			arg1 = (pMem[i + nPixelPerDCS] & pixelMask);

			if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
				arg1 = arg1 >> 4;
				arg2 = arg2 >> 4;
			}
			arg2 = 2048 - arg2;
			arg1 = 2048 - arg1;
			pixelData[i] = (double) abs(arg1) / (abs(arg2) + abs(arg1)) * MAX_DIST_VALUE + offset + 0.5;
		}
	}
	*data = pixelData;
	return nPixelPerDCS;
}

int calcPNDefNoPiDelayGetAmp(uint16_t **data) {
	int size = pruGetImage(data);
	uint16_t *pMem = *data;
	int nPixelPerDCS = size / 2;
	uint16_t pixelMask = calculatorGetPixelMask();

	for (i = 0; i < nPixelPerDCS; i++) {
		arg2 = (pMem[i] & pixelMask);
		arg1 = (pMem[i + nPixelPerDCS] & pixelMask);

		if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
			arg1 = arg1 >> 4;
			arg2 = arg2 >> 4;
		}
		arg2 = 2048 - arg2;
		arg1 = 2048 - arg1;

		pixelData[i] = abs(arg2) + abs(arg1); // amplitude divider = 1
	}
	*data = pixelData;
	return nPixelPerDCS;
}

int calcPNDefNoPiDelayGetInfo(uint16_t **data) {
	int offset = calibrationGetOffsetPhase();
	int size = pruGetImage(data);
	uint16_t *pMem = *data;
	uint16_t pixelMask = calculatorGetPixelMask();
	int nPixelPerDCS = size / 2;
	for (i = 0; i < nPixelPerDCS; i++) {
		arg2 = (pMem[i] & pixelMask);
		arg1 = (pMem[i + nPixelPerDCS] & pixelMask);

		if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
			arg1 = arg1 >> 4;
			arg2 = arg2 >> 4;
		}
		arg2 = 2048 - arg2;
		arg1 = 2048 - arg1;

		amplitude =  abs(arg2) + abs(arg1); // amplitude divider = 1
		if (hysteresisUpdate(i, amplitude)) {
			pixelData[i] = (double) abs(arg1) / (abs(arg2) + abs(arg2)) * MAX_DIST_VALUE + offset + 0.5;
		} else {
			pixelData[i] = LOW_AMPLITUDE;
		}
		pixelData[i + nPixelPerDCS] = amplitude;
	}
	*data = pixelData;
	return nPixelPerDCS * 2;
}
