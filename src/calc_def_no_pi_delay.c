#include "calc_def_no_pi_delay.h"
#include "calculation.h"
#include "calculator.h"
#include "pru.h"
#include "fp_atan2.h"
#include "calibration.h"
#include "hysteresis.h"
#include <math.h>

static uint16_t pixelData[328 * 252 * 2];
int32_t arg1;
int32_t arg2;
int16_t pixelDCS0;
int16_t pixelDCS1;
int16_t amplitude;
int32_t distance;
int i, r, c;

int calcDefNoPiDelayGetDist(uint16_t **data) {
	int offset = calibrationGetOffsetPhase();
	int size = pruGetImage(data);
	uint16_t *pMem = *data;
	int minAmplitude = pruGetMinAmplitude();
	uint16_t pixelMask = calculatorGetPixelMask();
	int nPixelPerDCS = size / 2;
	if (minAmplitude > 0) {
		for (i = 0; i < nPixelPerDCS; i++) {
			pixelDCS0 = pMem[i];
			pixelDCS1 = pMem[i + nPixelPerDCS];
			arg1 = pixelDCS1 & pixelMask;
			arg2 = pixelDCS0 & pixelMask;

			if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
				arg1 = arg1 >> 4;
				arg2 = arg2 >> 4;
			}

			arg1 = 2048 - arg1;
			arg2 = 2048 - arg2;

			amplitude = sqrt((arg1 * arg1) + (arg2 * arg2));
			if (hysteresisUpdate(i, amplitude)) {
				distance = fp_atan2(arg1, arg2);
				distance = ((distance * MAX_DIST_VALUE) / FP_M_2_PI) + offset + 0.5;
				pixelData[i] = (int16_t) ((distance + MODULO_SHIFT) % MAX_DIST_VALUE);
			} else {
				pixelData[i] = LOW_AMPLITUDE;
			}
		}
	} else {
		for (i = 0; i < nPixelPerDCS; i++) {
			pixelDCS0 = pMem[i];
			pixelDCS1 = pMem[i + nPixelPerDCS];

			arg1 = pixelDCS1 & pixelMask;
			arg2 = pixelDCS0 & pixelMask;

			if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
				arg1 = arg1 >> 4;
				arg2 = arg2 >> 4;
			}

			arg1 = 2048 - arg1;
			arg2 = 2048 - arg2;

			distance = fp_atan2(arg1, arg2);
			distance = ((distance * MAX_DIST_VALUE) / FP_M_2_PI) + offset + 0.5;
			pixelData[i] = (int16_t) ((distance + MODULO_SHIFT) % MAX_DIST_VALUE);
		}
	}
	*data = pixelData;
	return nPixelPerDCS;
}

int calcDefNoPiDelayGetAmp(uint16_t **data) {

	int size = pruGetImage(data);
	uint16_t *pMem = *data;
	int nPixelPerDCS = size / 2;
	uint16_t pixelMask = calculatorGetPixelMask();

	for (i = 0; i < nPixelPerDCS; i++) {
		arg1 = (pMem[i + nPixelPerDCS] & pixelMask);
		arg2 = (pMem[i] & pixelMask);

		if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
			arg1 = arg1 >> 4;
			arg2 = arg2 >> 4;
		}

		arg1 = 2048 - arg1;
		arg2 = 2048 - arg2;

		pixelData[i] = sqrt((arg1 * arg1) + (arg2 * arg2));
	}
	*data = pixelData;
	return nPixelPerDCS;
}

int calcDefNoPiDelayGetInfo(uint16_t **data) {
	int offset = calibrationGetOffsetPhase();
	int size = pruGetImage(data);
	uint16_t *pMem = *data;
	uint16_t pixelMask = calculatorGetPixelMask();
	int nPixelPerDCS = size / 2;
	for (i = 0; i < nPixelPerDCS; i++) {
		pixelDCS0 = pMem[i];
		pixelDCS1 = pMem[i + nPixelPerDCS];
		arg1 = (pixelDCS1 & pixelMask);
		arg2 = (pixelDCS0 & pixelMask);

		if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
			arg1 = arg1 >> 4;
			arg2 = arg2 >> 4;
		}

		arg1 = 2048 - arg1;
		arg2 = 2048 - arg2;

		amplitude = sqrt((arg1 * arg1) + (arg2 * arg2));
		if (hysteresisUpdate(i, amplitude)) {
			distance = fp_atan2(arg1, arg2);
			distance = ((distance * MAX_DIST_VALUE) / FP_M_2_PI) + offset + 0.5;
			pixelData[i] = (int16_t) ((distance + MODULO_SHIFT) % MAX_DIST_VALUE);
		} else {
			pixelData[i] = LOW_AMPLITUDE;
		}
		pixelData[i + nPixelPerDCS] = amplitude;
	}
	*data = pixelData;
	return nPixelPerDCS * 2;
}

