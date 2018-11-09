#include "flim.h"
#include "i2c.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>

/** \addtogroup flim
 * @{
 */

static uint16_t flimRepetitions = 1;
static uint16_t flimFlashWidth  = 0;


int16_t flimSetT1(uint16_t value) {
	return flimSetDelay(SKIP_FLIM_T1, value);
}

int16_t flimSetT2(uint16_t value) {
	return flimSetDelay(SKIP_FLIM_T2, value);
}

int16_t flimSetT3(uint16_t value) {
	return flimSetDelay(SKIP_FLIM_T3, value);
}

int16_t flimSetT4(uint16_t value) {
	return flimSetDelay(SKIP_FLIM_T4, value);
}

int16_t flimSetTREP(uint16_t value) {
	return flimSetDelay(SKIP_FLIM_TREP, value);
}

int16_t flimSetRepetitions(uint16_t value) {
	unsigned char *i2cData = malloc(1 * sizeof(unsigned char));
	flimRepetitions = value;
	int regValue;
	if (value <= 0) {
		regValue = 0;
	} else {
		regValue = value - 1;
	}
	i2cData[0] = (regValue >> 8) & 0xFF;
	i2cData[1] = regValue & 0xFF;
	i2c(configGetDeviceAddress(), 'w', 0xBA, 2, &i2cData);
	free(i2cData);
	return regValue;
}

uint16_t flimGetRepetitions(){
	return flimRepetitions;
}

int16_t flimSetFlashDelay(uint16_t value) {
	return flimSetDelay(SKIP_FLIM_FLASH_DELAY, value);
}

int16_t flimSetFlashWidth(uint16_t value) {
	flimFlashWidth = value;
	return flimSetDelay(SKIP_FLIM_FLASH_WIDTH, value);
}

uint16_t flimGetFlashWidth(){
	return flimFlashWidth;
}

int16_t flimSetDelay(flimModeCounterSkipBit bit, uint16_t value){
	unsigned int rAdr = 0x00;
	switch(bit){
	case SKIP_FLIM_FLASH_DELAY:
		rAdr = 0xBC;
		break;
	case SKIP_FLIM_FLASH_WIDTH:
		rAdr = 0xBE;
		break;
	case SKIP_FLIM_T1:
		rAdr = 0xB0;
		break;
	case SKIP_FLIM_T2:
		rAdr = 0xB2;
		break;
	case SKIP_FLIM_T3:
		rAdr = 0xB4;
		break;
	case SKIP_FLIM_T4:
		rAdr = 0xB6;
		break;
	case SKIP_FLIM_TREP:
		rAdr = 0xB8;
		break;
	}
	if (!value) {
		if (!((configGetPartType()==EPC502 || configGetPartType()==EPC502_OBS) && configGetIcVersion() < 3)){
			flimSetSkip(bit);
		}else{
			return -1;
		}
	}else{
		if (!((configGetPartType()==EPC502 || configGetPartType()==EPC502_OBS) && configGetIcVersion() < 3)){
			flimResetSkip(bit);
		}
		unsigned char *i2cData = malloc(2 * sizeof(unsigned char));
		--value;
		i2cData[0] = value >> 8;
		i2cData[1] = value & 0xFF;
		i2c(configGetDeviceAddress(), 'w', rAdr, 2, &i2cData);
		free(i2cData);
	}
	return 0;
}

void flimSetSkip(flimModeCounterSkipBit bit){
	unsigned char *i2cData = malloc(1 * sizeof(unsigned char));
	i2c(configGetDeviceAddress(), 'r', 0x2F, 1, &i2cData);
	i2cData[0] |= 	(1 << bit);
	i2c(configGetDeviceAddress(), 'w', 0x2F, 1, &i2cData);
	free(i2cData);
}

void flimResetSkip(flimModeCounterSkipBit bit){
	unsigned char *i2cData = malloc(1 * sizeof(unsigned char));
	i2c(configGetDeviceAddress(), 'r', 0x2F, 1, &i2cData);
	i2cData[0] &= ~(1 << bit);
	i2c(configGetDeviceAddress(), 'w', 0x2F, 1, &i2cData);
	free(i2cData);
}

//return ps
int16_t flimGetStep(){
	return configGetTModClk() * 1000;
}

/** @}*/
