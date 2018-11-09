#ifndef FP_ATAN2_LUT_H_
#define FP_ATAN2_LUT_H_

/**
 * Copyright (C) 2017 Espros Photonics Corporation
 *
 * This calculation of the atan is done using a table for one octant. This table
 * is calculated with the normal math library when the init function is called.
 * The scaling for the range (2 pi) must be passed to the init function. With the
 * scaling the output of the atan function is directly scaled to the desired range.
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

int32_t atan2lut_Init();
int32_t atan2_lut(int32_t y, int32_t x);


#ifdef __cplusplus
}
#endif



#endif // FP_ATAN2_LUT_H_
