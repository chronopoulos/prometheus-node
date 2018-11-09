/**
 * Copyright (C) 2017 Espros Photonics Corporation
 *
 *
 * @addtogroup atan
 *
 * This calculation of the atan is done using a table for one octant. This table
 * is calculated with the normal math library when the init function is called.
 * The scaling for the range (2 pi) must be passed to the init function. With the
 * scaling the output of the atan function is directly scaled to the desired range.
 *
 * @{
 */
#include "evalkit_constants.h"
#include "fp_atan2_lut.h"
#include <stddef.h>
#include <stdlib.h>
#include <math.h>

// Defines
#define RANGE_2PI      30000          //Fixed range for 2pi if the flexible range is not used
//#define NUM_LUT_VALUE  3750           //Number of values in the look-up table
//#define NUM_LUT_VALUE  2048           //Number of values in the look-up table
#define NUM_LUT_VALUE  4096

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

// Constants
const int32_t numLutValue = NUM_LUT_VALUE;      //Number of values in the look-up table (constant for calculations)

// Static variables
static uint16_t atanLUT[NUM_LUT_VALUE];         //Table for one octant

// Get the octant a coordinate pair is in.
#define OCTANTIFY(_x, _y, _o)                                   \
{                                                               \
    int _t; _o= 0;                                              \
    if(_y<  0)  {            _x= -_x;   _y= -_y; _o += MAX_DIST_VALUE_2; }     \
    if(_x<= 0)  { _t= _x;    _x=  _y;   _y= -_t; _o += MAX_DIST_VALUE_4; }     \
    if(_x<=_y)  { _t= _y-_x; _x= _x+_y; _y=  _t; _o += MAX_DIST_VALUE_8; }     \
}

/**
 * @brief Init the look-up table
 *
 * This function must be called before using the atan function. Otherwise it will
 * not calculate correct values. The range for 2pi can be setup with this function so
 * the result is directly scaled to the distance.
 * @return Status code, 0 = OK, else error
 */
int32_t atan2lut_Init(){
	//Fill the table (one octant) using the math library
	for (int i = 0; i < numLutValue; i++){

		//The first parameter loops, the second parameter stays fixed
		double actValPi = atan2((double)i, (double)(numLutValue - 1));

		//The output range of the math function was 0..2pi. Scale it here to 0..RangeInMillimeter
		actValPi = actValPi * ((double)(MAX_DIST_VALUE) / (2.0 * M_PI));

		//Finally put the value into the table
		atanLUT[i] = (uint16_t)(round(actValPi));
	}

	return 0;
}

/**
 * @brief Calculate the atan using the table
 *
 * This function calculates the atan of the given values. It uses the fixed range that is
 * defined with a constant. This function is faster than the function with flexible range, so
 * if the range (modulation frequency) is not changed, this function can be used.
 *
 * @param y coordinate
 * @param x coordinate
 */
int32_t atan2_lut(int32_t y, int32_t x){
	//Handle the special values
	if (y == 0){
		if (x >= 0)	return 0;
		else		return (MAX_DIST_VALUE / 2);  //Return Pi
	}

	int32_t octant;
	OCTANTIFY(x, y, octant);	//Check, in which octant we are. Swap values if needed.
	int32_t index = (y * NUM_LUT_VALUE) / x;	//Calculate the index to the look-up table
	//int32_t alfa = octant * (MAX_DIST_VALUE / 8);	//Calculate the angle of the octants
	return octant + atanLUT[index];	//Add the angle of the octants to the value of the look-up table
}


