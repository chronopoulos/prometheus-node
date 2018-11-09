#include "image_averaging.h"
#include "calculation.h"
#include "evalkit_constants.h"
#include "pru.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/// \addtogroup image_processing
/// @{

static unsigned int averageOver = 0;
static int n = 1;
static uint16_t lastFrame[328*252];

int imageSetNumberOfPicture(const unsigned int nPictures){
	if (nPictures <= 1){
		averageOver = 1;
	}else{
		averageOver = nPictures;
		n = 1;
	}
	return averageOver;
}

void imageAverage(uint16_t **data){
	if (averageOver <= 1){
		return;
	}
	uint16_t *pMem = *data;
	int nCols = pruGetNCols();
	int nRowsPerHalf = pruGetNRowsPerHalf();
	int size = nCols * nRowsPerHalf * 2;
	n = averageOver;

	for (int i = 0; i < size; i++){
		int32_t value = pMem[i];

		if(lastFrame[i] >= LOW_AMPLITUDE){
			lastFrame[i] = value;
		}

		if(value < LOW_AMPLITUDE){
			value = ((n-1) * (int)lastFrame[i] + (int)value) / n;
			pMem[i] = value;
		}

		lastFrame[i] = value;
	}

}

/// @}
