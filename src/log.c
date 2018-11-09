#include "read_out.h"
#include "i2c.h"
#include <stdlib.h>

/// \addtogroup output
/// @{


/*!
 Gets a dump of all registers
 @param deviceAddress i2c address of the chip (default 0x20)
 @returns data read from the chip
 */
void dumpAllRegisters(const int deviceAddress, int16_t *values) {
	unsigned char *i2cData = malloc(256 * sizeof(unsigned char));
	i2c(deviceAddress, 'r', 0x00, 256, &i2cData);
	int i;
	for (i = 0; i < 256; i++){
		values[i] = (int16_t) i2cData[i];
	}
	free(i2cData);
}

/// @}
