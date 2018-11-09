#include "calc_def_pi_delay.h"
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
uint16_t pixelDCS0;
uint16_t pixelDCS1;
uint16_t pixelDCS2;
uint16_t pixelDCS3;
int16_t amplitude;
int32_t distance;
int i, r, c;

int calcDefPiDelayGetDist(uint16_t **data) {
	int offset = calibrationGetOffsetPhase();
	int size = pruGetImage(data);

	uint16_t *pMem = *data;
	int32_t *calibrationMap = calibrationGetCorrectionMap();
	uint16_t pixelMask = calculatorGetPixelMask();
	int minAmplitude = pruGetMinAmplitude();
	int nPixelPerDCS = size / 4;
	if (minAmplitude > 0) {
		for (i = 0; i < nPixelPerDCS; i++) {
			pixelDCS0 = pMem[i];
			pixelDCS1 = pMem[i + nPixelPerDCS];
			pixelDCS2 = pMem[i + 2 * nPixelPerDCS];
			pixelDCS3 = pMem[i + 3 * nPixelPerDCS];
			arg1 = (pixelDCS3 & pixelMask) - (pixelDCS1 & pixelMask);
			arg2 = (pixelDCS2 & pixelMask) - (pixelDCS0 & pixelMask);

			if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
					arg1 = arg1 >> 4;
					arg2 = arg2 >> 4;
			}

			amplitude = sqrt((arg1 * arg1 / 4) + (arg2 * arg2 / 4));
			if (hysteresisUpdate(i, amplitude)) {
				distance = fp_atan2(arg1, arg2);
				distance = ((distance * MAX_DIST_VALUE) / FP_M_2_PI) + offset + calibrationMap[i] + 0.5;
				pixelData[i] = (int16_t)((distance + MODULO_SHIFT) % MAX_DIST_VALUE);
			} else {
				pixelData[i] = LOW_AMPLITUDE;
			}
		}
	} else {
		for (i = 0; i < nPixelPerDCS; i++) {
			pixelDCS0 = pMem[i];
			pixelDCS1 = pMem[i + nPixelPerDCS];
			pixelDCS2 = pMem[i + 2 * nPixelPerDCS];
			pixelDCS3 = pMem[i + 3 * nPixelPerDCS];
			arg1 = (pixelDCS3 & pixelMask) - (pixelDCS1 & pixelMask);
			arg2 = (pixelDCS2 & pixelMask) - (pixelDCS0 & pixelMask);

			if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
					arg1 = arg1 >> 4;
					arg2 = arg2 >> 4;
			}

			distance = fp_atan2(arg1, arg2);
			distance = ((distance * MAX_DIST_VALUE) / FP_M_2_PI) + offset + calibrationMap[i] + 0.5;
			pixelData[i] = (int16_t)((distance + MODULO_SHIFT) % MAX_DIST_VALUE);
		}
	}
	*data = pixelData;
	return nPixelPerDCS;
}

int calcDefPiDelayGetAmp(uint16_t **data) {
	int size = pruGetImage(data);
	uint16_t *pMem = *data;
	int nPixelPerDCS = size / 4;
	uint16_t pixelMask = calculatorGetPixelMask();

	for (i = 0; i < nPixelPerDCS; i++) {
		arg1 = (pMem[i + 3 * nPixelPerDCS] & pixelMask) - (pMem[i + nPixelPerDCS] & pixelMask);
		arg2 = (pMem[i + 2 * nPixelPerDCS] & pixelMask) - (pMem[i] & pixelMask);

		if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
				arg1 = arg1 >> 4;
				arg2 = arg2 >> 4;
		}

		pixelData[i] = sqrt((arg1 * arg1 / 4) + (arg2 * arg2 / 4));
	}
	*data = pixelData;
	return nPixelPerDCS;
}

int calcDefPiDelayGetInfo(uint16_t **data) {
	int offset = calibrationGetOffsetPhase();
	int size = pruGetImage(data);
	uint16_t *pMem = *data;
	int32_t *calibrationMap = calibrationGetCorrectionMap();
	uint16_t pixelMask = calculatorGetPixelMask();
	int nPixelPerDCS = size / 4;
	for (i = 0; i < nPixelPerDCS; i++) {
		pixelDCS0 = pMem[i];
		pixelDCS1 = pMem[i + nPixelPerDCS];
		pixelDCS2 = pMem[i + 2 * nPixelPerDCS];
		pixelDCS3 = pMem[i + 3 * nPixelPerDCS];
		arg1 = (pixelDCS3 & pixelMask) - (pixelDCS1 & pixelMask);
		arg2 = (pixelDCS2 & pixelMask) - (pixelDCS0 & pixelMask);

		if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
			arg1 = arg1 >> 4;
			arg2 = arg2 >> 4;
		}

		amplitude = sqrt((arg1 * arg1 / 4) + (arg2 * arg2 / 4));
		if (hysteresisUpdate(i, amplitude)) {
			distance = fp_atan2(arg1, arg2);
			distance = ((distance * MAX_DIST_VALUE) / FP_M_2_PI) + offset + calibrationMap[i] + 0.5;
			pixelData[i] = (int16_t)((distance + MODULO_SHIFT) % MAX_DIST_VALUE);
		} else {
			pixelData[i] = LOW_AMPLITUDE;
		}
		pixelData[i + nPixelPerDCS] = amplitude;
	}
	*data = pixelData;
	return nPixelPerDCS * 2;
}
