#include "chip_info.h"
#include "config.h"
#include "i2c.h"
#include <stdlib.h>

/// \addtogroup chip_info
/// @{


int16_t chipInfoGetInfo(const int deviceAddress, int16_t *chipInfo){
	unsigned char *i2cData = malloc(2 * sizeof(unsigned char));
	i2c(deviceAddress, 'r', 0x00, 2, &i2cData);
	chipInfo[0] = i2cData[0];
	chipInfo[1] = i2cData[1];
	i2cData[0] = 0xF5;
	i2c(deviceAddress, 'w', 0x11, 1, &i2cData);
	i2c(deviceAddress, 'r', 0x12, 1, &i2cData);
	chipInfo[2] = i2cData[0];
	i2c(deviceAddress, 'r', 0x12, 1, &i2cData);
	int16_t waferIDMSB = i2cData[0] << 8;
	i2c(deviceAddress, 'r', 0x12, 1, &i2cData);
	chipInfo[3] = i2cData[0] + waferIDMSB;
	i2c(deviceAddress, 'r', 0x12, 1, &i2cData);
	int16_t chipIDMSB = i2cData[0] << 8;
	i2c(deviceAddress, 'r', 0x12, 1, &i2cData);
	chipInfo[4] = i2cData[0] + chipIDMSB;
	i2c(deviceAddress, 'r', 0x12, 1, &i2cData);
	chipInfo[5] = i2cData[0];
	i2c(deviceAddress, 'r', 0x12, 1, &i2cData);
	chipInfo[6] = i2cData[0];
	chipInfo[7] = configGetSequencerVersion();
	free(i2cData);
	return 8;
}


///@}
