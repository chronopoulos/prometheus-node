
#define _GNU_SOURCE

#include "i2c.h"
#include "is5351A_clkGEN.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <math.h>
#include <time.h>
#include <unistd.h>

#include "../include/ref_gen_registers.h"

//GLOBAL VAR DECLARATION START***********************************************************************
typedef struct{
	uint8_t address;
}is5351A_t;

//GLOBAL VAR DECLARATION START***********************************************************************

//GLOBAL VAR INITALISATION START*********************************************************************

static is5351A_t ic1 = {	.address = 0x60};

//LOCAL FUN DECLARTATION END**********************************************************************


/*!
 Initializes the clock gen.
 */
int16_t initClkGen(int clkMhz){
	unsigned char *i2cData = malloc(sizeof(unsigned char));
	i2cData[0]=0x00;
	int i;

	switch(clkMhz){
		case MHZ_50:
			for(i=0;i<SI5351A_REVB_REG_CONFIG_NUM_REGS;i++){
				i2cData[0] = si5351a_revb_registers200Mhz[i].value;
				printf("address: %d value: %d \n",si5351a_revb_registers200Mhz[i].address,i2cData[0]);
				i2c(ic1.address, 'w', si5351a_revb_registers200Mhz[i].address, 1, &i2cData); // Set clk gen registers
				usleep(10000); // wait for 10 ms
				i2cData[0]=0x00;
				i2c(ic1.address, 'r', si5351a_revb_registers200Mhz[i].address, 1, &i2cData); // Set clk gen registers
				printf("address: %d value: %d \n",si5351a_revb_registers200Mhz[i].address,i2cData[0]);
			}
			printf("RefGen out [Mhz]: %d counter i: %d \n",clkMhz,i);
			break;
		case MHZ_40:
			for(i=0;i<SI5351A_REVB_REG_CONFIG_NUM_REGS;i++){
				i2cData[0] = si5351a_revb_registers160Mhz[i].value;
				printf("address: %d value: %d \n",si5351a_revb_registers160Mhz[i].address,i2cData[0]);
				i2c(ic1.address, 'w', si5351a_revb_registers160Mhz[i].address, 1, &i2cData); // Set clk gen registers
				usleep(10000); // wait for 10 ms
				i2cData[0]=0x00;
				i2c(ic1.address, 'r', si5351a_revb_registers160Mhz[i].address, 1, &i2cData); // Set clk gen registers
				printf("address: %d value: %d \n",si5351a_revb_registers160Mhz[i].address,i2cData[0]);
			}
			printf("RefGen out [Mhz]: %d counter i: %d \n",clkMhz,i);
			break;
		case MHZ_36:
			for(i=0;i<SI5351A_REVB_REG_CONFIG_NUM_REGS;i++){
				i2cData[0] = si5351a_revb_registers144Mhz[i].value;
				printf("address: %d value: %d \n",si5351a_revb_registers144Mhz[i].address,i2cData[0]);
				i2c(ic1.address, 'w', si5351a_revb_registers144Mhz[i].address, 1, &i2cData); // Set clk gen registers
				usleep(10000); // wait for 10 ms
				i2cData[0]=0x00;
				i2c(ic1.address, 'r', si5351a_revb_registers144Mhz[i].address, 1, &i2cData); // Set clk gen registers
				printf("address: %d value: %d \n",si5351a_revb_registers144Mhz[i].address,i2cData[0]);
			}
			printf("RefGen out [Mhz]: %d counter i: %d \n",clkMhz,i);
			break;
		case MHZ_30:
			for(i=0;i<SI5351A_REVB_REG_CONFIG_NUM_REGS;i++){
				i2cData[0] = si5351a_revb_registers120Mhz[i].value;
				printf("address: %d value: %d \n",si5351a_revb_registers120Mhz[i].address,i2cData[0]);
				i2c(ic1.address, 'w', si5351a_revb_registers120Mhz[i].address, 1, &i2cData); // Set clk gen registers
				usleep(10000); // wait for 10 ms
				i2cData[0]=0x00;
				i2c(ic1.address, 'r', si5351a_revb_registers120Mhz[i].address, 1, &i2cData); // Set clk gen registers
				printf("address: %d value: %d \n",si5351a_revb_registers120Mhz[i].address,i2cData[0]);
			}
			printf("RefGen out [Mhz]: %d counter i: %d \n",clkMhz,i);
			break;
		case MHZ_24:
			for(i=0;i<SI5351A_REVB_REG_CONFIG_NUM_REGS;i++){
				i2cData[0] = si5351a_revb_registers96Mhz[i].value;
				printf("address: %d value: %d \n",si5351a_revb_registers96Mhz[i].address,i2cData[0]);
				i2c(ic1.address, 'w', si5351a_revb_registers96Mhz[i].address, 1, &i2cData); // Set clk gen registers
				usleep(10000); // wait for 10 ms
				i2cData[0]=0x00;
				i2c(ic1.address, 'r', si5351a_revb_registers96Mhz[i].address, 1, &i2cData); // Set clk gen registers
				printf("address: %d value: %d \n",si5351a_revb_registers96Mhz[i].address,i2cData[0]);
			}
			printf("RefGen out [Mhz]: %d counter i: %d \n",clkMhz,i);
			break;
		default:
			break;
	}

	i2cData[0] = si5351a_revb_registersAfterWrite[0].value;
	i2c(ic1.address, 'w', si5351a_revb_registersAfterWrite[0].address, 1, &i2cData); // Set clk gen registers
	printf("pll reset with address: %d value: %d \n",si5351a_revb_registersAfterWrite[0].address,i2cData[0]);
	usleep(10000); // wait for 10 ms

	free(i2cData);
	return clkMhz;
}
