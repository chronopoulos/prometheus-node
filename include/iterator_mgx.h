#ifndef ITERATOR_MGX_H_
#define ITERATOR_MGX_H_

#include <stdint.h>
#include <stdbool.h>

struct Position{
	uint16_t x;
	uint8_t y;
	uint32_t indexMemory;
	uint32_t indexSorted;
	uint32_t indexCalibration;
	bool top;
};

void iteratorMGXInit(uint32_t numberOfPixelsPerDCS, uint16_t width, uint8_t height);
bool iteratorMGXHasNext();
struct Position iteratorMGXNext();

#endif /* ITERATOR_MGX_H_ */
