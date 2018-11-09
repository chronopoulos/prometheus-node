#ifndef _IMAGE_CORRECTION_H_
#define _IMAGE_CORRECTION_H_

#include <stdint.h>

typedef struct pixelCorrection {
	uint8_t x;
	uint8_t y;
	uint32_t pos;
	uint8_t neighbour;
}pixelCorrection;

void imageCorrectionLoad(const char *filename);
int imageCorrectionEnable(const unsigned int state);
void imageCorrectionTransform();
void imageCorrect(uint16_t **data);
void imageCorrectCopyPixel(const pixelCorrection pixel, uint16_t **data);
void imageCorrectAveragePixel(const unsigned int pos, uint16_t **data);

#endif
