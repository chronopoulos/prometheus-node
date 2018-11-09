#ifndef _CALC_DEF_PI_DELAY_H_
#define _CALC_DEF_PI_DELAY_H_

#include <stdint.h>

int calcDefPiDelayGetDist(uint16_t **data);
int calcDefPiDelayGetAmp(uint16_t **data);
int calcDefPiDelayGetInfo(uint16_t **data);
int calcDefPiDelayEnableImageProcessing(int state);


#endif
