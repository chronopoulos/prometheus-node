#include "version.h"


/// \addtogroup output
/// @{


static uint8_t major = 2;	//max 3
static uint8_t minor = 12;	//max 99
static uint8_t patch = 0;	//max 99

/*!
Getter function for the version
@returns the version
 */
int16_t getVersion(){
	return major * 10000 + minor * 100 + patch;
}

unsigned int getMajor(){
	return major;
}

unsigned int getMinor(){
	return minor;
}

unsigned int getPatch(){
	return patch;
}


/// @}
