#include "hysteresis.h"
#include "evalkit_constants.h"
#include <stdio.h>
#include <string.h>

/// \addtogroup hysteresis
/// @{

static unsigned char hysteresisMap[MAX_NUM_PIX];
static int hysteresis = 0;
static int upperThreshold = 0;
static int lowerThreshold = 0;

int hysteresisUpdate(const int position, const int value) {
	if (value >= upperThreshold) {
		hysteresisMap[position] = 1;
	} else if (value < lowerThreshold) {
		hysteresisMap[position] = 0;
	}
	return hysteresisMap[position];
}

int hysteresisSetThresholds(const int minAmplitude, const int value) {
	hysteresis = value;
	hysteresisUpdateThresholds(minAmplitude);
	return hysteresis;
}

void hysteresisUpdateThresholds(const int minAmplitude){
	upperThreshold = minAmplitude;
	lowerThreshold = minAmplitude - hysteresis;
	memset(hysteresisMap, 0, MAX_NUM_PIX * sizeof(unsigned char));
}

int hysteresisIsEnabled(const int position) {
	return hysteresisMap[position];
}


/// @}
