#ifndef _IMAGE_DIFFERENCE_H_
#define _IMAGE_DIFFERENCE_H_

#include <stdint.h>

void imageDifferenceInit();
int imageDifferenceSetThreshold(const unsigned int cm);
void imageDifference(uint16_t **data);

#endif
