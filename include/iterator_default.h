#ifndef ITERATOR_DEFAULT_H_
#define ITERATOR_DEFAULT_H_

#include <stdint.h>
#include <stdbool.h>

struct Position{
	uint16_t x;
	uint8_t y;
	uint32_t indexMemory;
	uint32_t indexSorted;
};

void iteratorDefaultInit(uint32_t numberOfPixelsPerDCS, uint16_t width, uint8_t height);
bool iteratorDefaultHasNext();
struct Position iteratorDefaultNext();

#endif /* ITERATOR_DEFAULT_H_ */
