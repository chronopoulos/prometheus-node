#include "image_processing.h"
#include "pru.h"
#include "calculation.h"
#include "evalkit_constants.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/// \addtogroup image_processing
/// @{


static int imageProcessing = 0;
static int averageOver = 5;

static int currPicture;
static uint16_t **storedFrames = NULL;
static uint16_t actualFrame[MAX_NUM_PIX];

void imageInitAveragePicture(){
	if(storedFrames){
		int f;
		for (f = 0; f < averageOver; f++) {
			free(storedFrames[f]);
		}
		free(storedFrames);
	}
	currPicture = -averageOver;
	storedFrames = malloc(averageOver * sizeof(int16_t *));
	int i;
	for (i = 0; i < averageOver; i++){
		storedFrames[i] = malloc(MAX_NUM_PIX * sizeof(int16_t));
	}

	//free???
}

void imageAveragePicture(uint16_t **data){
	printf("imageAveragePicture\n");
	int nCols = pruGetNCols();
	int nRowsPerHalf = pruGetNRowsPerHalf();
	int size = nCols * nRowsPerHalf * 2;
	if (currPicture < 0){ //
		memcpy(storedFrames[currPicture + averageOver], data, size * sizeof(int16_t));
		currPicture++;
	}
	else{
		memcpy(storedFrames[currPicture], data, size * sizeof(int16_t));
		int i;
		int c;
		for (i = 0; i < size; i++){
			uint16_t value = 0;
			for (c = 0; c < averageOver; c++){
				value += storedFrames[c][i];
			}
			*data[i] = value / averageOver;
		}
		currPicture = ((currPicture + 1) % averageOver);
	}
	printf("current Picture = %i\n", currPicture);
}

// 1
void imageAverageFourConnectivity(uint16_t **data) {
	// me = (me + west + north + east + south) / x
	int p;
	int pixelCounter;
	int nCols = pruGetNCols();
	int nRowsPerHalf = pruGetNRowsPerHalf();
	int size = nCols * nRowsPerHalf * 2;
	memcpy(actualFrame, *data, size * sizeof(int16_t));
	//		//middle part
	*data[0] = (actualFrame[0] + actualFrame[2 * nCols] + actualFrame[1] + actualFrame[nCols]) / 4;
	*data[nCols - 1] = (actualFrame[nCols - 1] + actualFrame[nCols - 2] + actualFrame[3 * nCols - 1] + actualFrame[2 * nCols - 1]) / 4;
	for (p = 1; p < nCols - 1; p++) {
		*data[p] = (actualFrame[p] + actualFrame[p - 1] + actualFrame[p + 2 * nCols] + actualFrame[p + 1] + actualFrame[p + nCols]) / 5;
	}
	*data[nCols] = (actualFrame[nCols] + actualFrame[0] + +actualFrame[nCols + 1] + actualFrame[3 * nCols]) / 4;
	*data[2 * nCols - 1] = (actualFrame[2 * nCols - 1] + actualFrame[2 * nCols - 2] + actualFrame[nCols - 1] + actualFrame[4 * nCols - 1]) / 4;
	for (p = nCols; p < 2 * nCols - 1; p++) {
		*data[p] = (actualFrame[p] + actualFrame[p - 1] + actualFrame[p - nCols] + actualFrame[p + 1] + actualFrame[p + 2 * nCols]) / 5;
	}
	//
	// normal part
	for (p = 2 * nCols; p < (nRowsPerHalf - 1) * 2 * nCols; p++) {
		//check if left
		if (p % nCols == 0) {
			*data[p] = (actualFrame[p] + actualFrame[p - 2 * nCols] + actualFrame[p + 1] + actualFrame[p + 2 * nCols]) / 4;
		}
		//check if right
		else if (p % nCols == nCols - 1) {
			*data[p] = (actualFrame[p] + actualFrame[p - 1] + actualFrame[p - 2 * nCols] + actualFrame[p + 2 * nCols]) / 4;
		} else {
			pixelCounter = 0;
			if (actualFrame[p] != LOW_AMPLITUDE) {
				pixelCounter++;
			}
			if (actualFrame[p - 1] != LOW_AMPLITUDE) {
				*data[p] += actualFrame[p - 1];
				pixelCounter++;
			}
			if (actualFrame[p - 2 * nCols] != LOW_AMPLITUDE) {
				*data[p] += actualFrame[p - 2 * nCols];
				pixelCounter++;
			}
			if (actualFrame[p + 1] != LOW_AMPLITUDE) {
				*data[p] += actualFrame[p + 1];
				pixelCounter++;
			}
			if (actualFrame[p + 2 * nCols] != LOW_AMPLITUDE) {
				*data[p] += actualFrame[p + 2 * nCols];
				pixelCounter++;
			}
			if (pixelCounter < 3) {
				*data[p] = LOW_AMPLITUDE;
			} else {
				*data[p] /= pixelCounter;
			}
		}
	}
	//
	//end part
}

// 2
void imageAverageEightConnectivity(uint16_t **data) {
	int p;
	int pixelCounter;
	int nCols = pruGetNCols();
	int nRowsPerHalf = pruGetNRowsPerHalf();
	int size = nCols * nRowsPerHalf * 2;
	memcpy(actualFrame, *data, size * sizeof(int16_t));
//	// middle part
//	data[0] = (actualFrame[0] + actualFrame[2 * nCols] + + actualFrame[2 * nCols + 1] + actualFrame[1] + actualFrame[nCols + 1] + actualFrame[nCols]) / 6;
//	data[nCols - 1] = (actualFrame[nCols - 1] + actualFrame[nCols - 2] + actualFrame[3* nCols - 2] + actualFrame[3 * nCols - 1] + actualFrame[2 * nCols - 1] + actualFrame[2 * nCols - 2]) / 6;
//	for (p = 1; p < nCols - 1; p++) {
//		data[p] = (actualFrame[p] + actualFrame[p - 1] + actualFrame[p + 2 * nCols - 1] + actualFrame[p + 2 * nCols] + actualFrame[p + 2 * nCols + 1] + actualFrame[p + 1] + actualFrame[p + nCols + 1] + actualFrame[p + nCols] + actualFrame[p + nCols - 1]) / 9;
//	}
//
//	data[nCols] = (actualFrame[nCols] + actualFrame[0] + actualFrame[1] + +actualFrame[nCols + 1] + actualFrame[3 * nCols + 1] + actualFrame[3 * nCols] + actualFrame[3 * nCols - 1]) / 6;
//	data[2 * nCols - 1] = (actualFrame[2 * nCols - 1] + actualFrame[2 * nCols - 2] + actualFrame[nCols - 2] + actualFrame[nCols - 1] + actualFrame[4 * nCols - 1]) / 4;
//	for (p = nCols; p < 2 * nCols - 1; p++) {
//		data[p] = (actualFrame[p] + actualFrame[p - 1] + actualFrame[p - nCols] + actualFrame[p + 1] + actualFrame[p + 2 * nCols]) / 5;
//	}
//	//
	// normal part
	for (p = 2 * nCols; p < (nRowsPerHalf - 1) * 2 * nCols; p++) {
		//check if left
		if (p % nCols == 0) {
//			data[p] = (actualFrame[p] + actualFrame[p - 2 * nCols] + actualFrame[p + 1] + actualFrame[p + 2 * nCols]) / 4;
		}
		//check if right
		else if (p % nCols == nCols - 1) {
//			data[p] = (actualFrame[p] + actualFrame[p - 1] + actualFrame[p - 2 * nCols] + actualFrame[p + 2 * nCols]) / 4;
		} else {
			pixelCounter = 0;
			if (actualFrame[p] != LOW_AMPLITUDE) {
				pixelCounter++;
			}
			if (actualFrame[p - 1] != LOW_AMPLITUDE) {
				*data[p] += actualFrame[p - 1];
				pixelCounter++;
			}
			if (actualFrame[p + 2 * nCols - 1] != LOW_AMPLITUDE) {
				*data[p] += actualFrame[p + 2 * nCols - 1];
				pixelCounter++;
			}
			if (actualFrame[p + 2 * nCols] != LOW_AMPLITUDE) {
				*data[p] += actualFrame[p + 2 * nCols];
				pixelCounter++;
			}
			if (actualFrame[p + 2 * nCols + 1] != LOW_AMPLITUDE) {
				*data[p] += actualFrame[p + 2 * nCols + 1];
				pixelCounter++;
			}
			if (actualFrame[p + 1] != LOW_AMPLITUDE) {
				*data[p] += actualFrame[p + 1];
				pixelCounter++;
			}
			if (actualFrame[p + 2 * nCols + 1] != LOW_AMPLITUDE) {
				*data[p] += *data[p + 2 * nCols + 1];
				pixelCounter++;
			}
			if (actualFrame[p + 2 * nCols] != LOW_AMPLITUDE) {
				*data[p] += *data[p + 2 * nCols];
				pixelCounter++;
			}
			if (actualFrame[p + 2 * nCols - 1] != LOW_AMPLITUDE) {
				*data[p] += *data[p + 2 * nCols - 1];
				pixelCounter++;
			}
			if (pixelCounter < 6) {
				*data[p] = LOW_AMPLITUDE;
			} else {
				*data[p] /= pixelCounter;
			}
		}
	}
	//
	//end part
}

int imageSetProcessing(unsigned int mode) {
	if (mode > 3) {
		mode = 3;
	}
	imageProcessing = mode;
	return imageProcessing;
}

void imageProcess(uint16_t **data) {
	switch (imageProcessing) {
	case 1:
		imageAverageFourConnectivity(data);
		break;
	case 2:
		imageAverageEightConnectivity(data);
		break;
	case 0:
	default:
		//nothing
		break;
	}
}


/// @}
