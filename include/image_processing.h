#ifndef _IMAGE_PROCESSING_H_
#define _IMAGE_PROCESSING_H_

#include <stdint.h>

void imageInitAveragePicture();
void imageAveragePicture(uint16_t **data);
void imageAverageFourConnectivity(uint16_t **data);
void imageAverageEightConnectivity(uint16_t **data);
int imageSetProcessing(unsigned int mode);
void imageProcess(uint16_t **data);

#endif
