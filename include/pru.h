#ifndef PRU_H_
#define PRU_H_

#include "config.h"
#include <stdint.h>

#define PRU_CONF_SIZE 12
#define PRU_NUM 1 /*! Defines which PRU is used*/
#define REQUIRED_MEM_SIZE 0x100000

int pruInit(const unsigned int addressOfDevice);
void pruSetSize(const unsigned int nOfColumns, const unsigned int nofRowsPerHalf);
int pruGetNumberOfHalves();
void pruSetDCS(const unsigned int nOfDCS);
void pruSetRowReduction(int reduction);
int pruGetRowReduction();
void pruSetColReduction(int reduction);
int pruGetColReduction();
int pruGetMinAmplitude();
int pruGetNRowsPerHalf();
int pruGetNCols();
int pruGetNDCS();
void pruSetMinAmplitude(const unsigned int minAmp);
int pruGetImage(uint16_t **data);
void pruPrime(void);
int pruCollect(uint16_t**);
int pruRelease();

#endif
