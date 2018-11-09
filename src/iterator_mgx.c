#include "iterator_mgx.h"
#include "pru.h"


/// \addtogroup iterator
/// @{


static uint32_t index = 0;
static uint32_t indexCal = 0;
static uint32_t nPxDcs;
static uint16_t h;
static uint16_t w;
static uint32_t x = 0, y = 0;
static uint16_t nHalves;

void iteratorMGXInit(uint32_t numberOfPixelsPerDCS, uint16_t width, uint8_t height){
	nPxDcs = numberOfPixelsPerDCS;
	w = width;
	h = height;
	index = 0;
	indexCal = 0;
	x = 0;
	y = 0;
	nHalves = pruGetNumberOfHalves();
}

bool iteratorMGXHasNext(){
	return index < nPxDcs;
}

struct Position iteratorMGXNext(){
	struct Position pos;
	if (nHalves == 2) {
		if (x < w){
			pos.x = x;
			pos.y = h / 4 - 1 - y;
			pos.top = true;
		}else{
			pos.x = x - w;
			pos.y = h / 4 + y;
			pos.top = false;
		}
	} else {
		pos.x = x;
		pos.y = h/2 - 1 - y;
		pos.top = true;
	}
	pos.indexMemory = index;
	pos.indexCalibration = indexCal;
	pos.indexSorted = pos.y * w + pos.x;

	++x;
	if (x == nHalves * w){
		x = 0;
		++y;
		index += nHalves * w;
	}
	++index;
	++indexCal;

	return pos;
}

/// @}
