#ifndef _TEMPERATURE_H_
#define _TEMPERATURE_H_

#include <stdint.h>

void temperatureInit();
int16_t  temperatureGetTemperatures(const int deviceAddress, int16_t *temps);
int16_t  temperatureGetTemperature(const int deviceAddress);
uint16_t temperatureGetTemperatureROI(const int deviceAddress, int16_t *temps);
uint16_t temperatureGetTemperatureROI_EPC660_CHIP7(const int deviceAddress, int16_t *temps);
double   temperatureGetAveragedChipTemperature(const int deviceAddress);
int16_t  temperatureGetTemperatureIllumination(int16_t *temps);
int16_t  temperatureGetLastTempChip();
int16_t  temperatureGetLastTempIllumination();
int16_t  temperatureGetTemperatureChip(const int deviceAddress, int16_t *temps);
int16_t  temperatureGetTemperatureRegister(const int deviceAddress, int16_t *temps);
int16_t  temperatureGetTemperature635(const int deviceAddress, int16_t *temps);
int16_t  temperatureKalmanFilter(int16_t *temps, int size);
uint16_t temperatureGetGrayscalePixel(int index);
uint16_t* temperatureGetGrayscaleData();

#endif
