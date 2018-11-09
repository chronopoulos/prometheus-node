#include "iterator_default.h"
#include "pru.h"


/// \addtogroup iterator
/// @{


static uint32_t index = 0;
static uint32_t nPxDcs;
static uint16_t h;
static uint16_t w;
static uint32_t x = 0, y = 0;
static uint16_t nHalves;

void iteratorDefaultInit(uint32_t numberOfPixelsPerDCS, uint16_t width, uint8_t height){
	nPxDcs = numberOfPixelsPerDCS;
	w = width;
	h = height;
	index = 0;
	x = 0;
	y = 0;
	nHalves = pruGetNumberOfHalves();
}

bool iteratorDefaultHasNext(){
	return index < nPxDcs;
}

struct Position iteratorDefaultNext(){
	struct Position pos;
	if (nHalves == 2) {
		if (x < w){
			pos.x = x;
			pos.y = h / 2 - 1 - y;
		}else{
			pos.x = x - w;
			pos.y = h / 2 + y;
		}
	} else {
		pos.x = x;
		pos.y = h - 1 - y;
	}

	pos.indexMemory = index;
	pos.indexSorted = pos.y * w + pos.x;

	++x;
	if (x == nHalves * w){
		x = 0;
		++y;
	}
	++index;

	return pos;
}

/// @}
