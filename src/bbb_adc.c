
#define _XOPEN_SOURCE 500

#include "bbb_adc.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>


/// \addtogroup BBB_ADC
/// @{

#define ADC_MAX_VALUE			4095
#define ADC_MAX_VOLTAGE_MV		1800

int adcOpen(unsigned int adc) {
	if (adc < 0 || adc > 7) {
		return -1;
	}
	char path[49];
	int fd;
	sprintf(path, "/sys/bus/iio/devices/iio:device0/in_voltage%u_raw", adc);
	fd = open(path, O_RDONLY);
	if (fd < 0) {
		return -1;
	}
	return fd;
}

//output 0-4095
int adcGetValue(unsigned int fd) {
	char adcValue[5];
	if (pread(fd, adcValue, 4, 0) < 0) {
		return -1;
	}
	adcValue[4] = '\0';
	int adcInt = strtol(adcValue, NULL, 10);
	return adcInt;
}

//output 0 - 1800mV
int adcGetValueMillivolts(unsigned int fd) {
	return (ADC_MAX_VOLTAGE_MV * adcGetValue(fd) / ADC_MAX_VALUE);
}

int adcGetValueMicroAmpere(unsigned int fd, unsigned int rOhm) {
	if (rOhm < 1) {
		return -1;
	}
	return 1000 * adcGetValueMillivolts(fd) / rOhm;
}

/// @}


