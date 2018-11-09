#include "image_difference.h"
#include "evalkit_constants.h"
#include "pru.h"
#include "logger.h"
#include "calculation.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/// \addtogroup image_processing
/// @{


static uint16_t storedFrame[328*252];
static int init = 0;
static int threshold = 0;

void imageDifferenceInit(){
	init = 0;
}

int imageDifferenceSetThreshold(const unsigned int cm){
	//TODO: cm to phase
	threshold = cm;
	init = 0;
	return threshold;
}

void imageDifference(uint16_t **data){
	if (!threshold){
		return;
	}
	uint16_t *pMem = *data;
	int nCols = pruGetNCols();
	int nRowsPerHalf = pruGetNRowsPerHalf();
	int size = nCols * nRowsPerHalf * 2;
	if (!init){
		memcpy(storedFrame, pMem, size * sizeof(int16_t));
		init = 1;
	}else{
		int i;
		for (i = 0; i < size; i++){
			int32_t value = abs(pMem[i] - storedFrame[i]);
			if (value < threshold){
				pMem[i] = LOW_AMPLITUDE;
			}
		}
	}
}

/// @}
