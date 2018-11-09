#ifndef _ADC_H_
#define _ADC_H_

int adcOpen(unsigned int adc);
int adcGetValue(unsigned int fd);
int adcGetValueMillivolts(unsigned int fd);
int adcGetValueMicroAmpere(unsigned int fd, unsigned int rOhm);

#endif
