#include "bbb_led.h"
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


/// \addtogroup BBB_LED
/// @{


/*!
Enables or disables the LED (blue LEDs on the BBB, LED3 = closest to the network connector)
 @param ledId ID of the LED
 @param state 1 for enable, 0 for disable
 @returns On success, 0, on error -1
 */
int bbbLEDEnable(const unsigned int ledId, const unsigned int state) {
	int fd;
	char * pathToLEDFile = "/sys/class/leds/beaglebone:green:usr0/brightness";

	switch (ledId) {
	case 0:
		break;
	case 1:
		pathToLEDFile = "/sys/class/leds/beaglebone:green:usr1/brightness";
		break;
	case 2:
		pathToLEDFile = "/sys/class/leds/beaglebone:green:usr2/brightness";
		break;
	case 3:
		pathToLEDFile = "/sys/class/leds/beaglebone:green:usr3/brightness";
		break;
	default:
		printf("ERROR: Invalid LED ID (only 0 to 3 allowed)\n");
		return -1;
	}

	fd = open(pathToLEDFile, O_WRONLY);
	if (fd < 0) {
		printf("ERROR: Opening file not possible.\n");
		return -1;
	}

	if (state) {
		if (write(fd, "1", 1) < 0) {
			close(fd);
			return -1;
		}
	} else {
		if (write(fd, "0", 1) < 0) {
			close(fd);
			return -1;
		}
	}
	close(fd);

	return 0;
}

/// @}
