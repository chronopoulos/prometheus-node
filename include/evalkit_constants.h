#ifndef _EVALKIT_CONSTANTS_H_
#define _EVALKIT_CONSTANTS_H_

// pru / chip constants
#define MAX_NUMBER_OF_COLS			328
#define MAX_NUMBER_OF_ROWS			252
#define MAX_NUMBER_OF_DOUBLE_ROWS	126

// server constants
#define TCP_PORT_NO 				50660
#define TCP_BUFFER_SIZE 			256
#define MAX_COMMAND_ARGUMENTS 		256
#define MAX_COMMAND_ARGUMENT_LENGTH	50
#define MAX_NUM_PIX			        82656	//328 * 252

#define MAX_PHASE                   30000.0
#define MAX_DIST_VALUE 				30000
#define MODULO_SHIFT 				300000
/*#define LOW_AMPLITUDE 			32500
#define SATURATION 					32600
#define ADC_OVERFLOW 				32700*/

#define LOW_AMPLITUDE 				65300
#define SATURATION 					65400
#define ADC_OVERFLOW 				65500

#define MODULO_SHIFT_PI             MAX_DIST_VALUE/2 + MODULO_SHIFT
#define MAX_DIST_VALUE_OCTANT       MAX_DIST_VALUE / 8

#define MAX_DIST_VALUE_2			MAX_DIST_VALUE / 2
#define MAX_DIST_VALUE_4			MAX_DIST_VALUE / 4
#define MAX_DIST_VALUE_8			MAX_DIST_VALUE / 8


//#define FP_M_2_PI					3217.0 	//! 2 PI in fp_s22_9 format
//#define FP_M_PI					1608.5


#endif
