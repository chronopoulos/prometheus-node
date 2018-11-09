#ifndef _IMAGE_AVERAGING_H_
#define _IMAGE_AVERAGING_H_

#include <stdint.h>

int imageSetNumberOfPicture(const unsigned int averageOver);
void imageAverage(uint16_t **data);

#endif
