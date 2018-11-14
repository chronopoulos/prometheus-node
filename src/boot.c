
#define _XOPEN_SOURCE 500

#include "boot.h"
#include "evalkit_gpio.h"
#include "gpio.h"
#include "i2c.h"
#include "logger.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <registers.h>

/// \addtogroup boot
/// @{

#define N_BOOT_ATTEMPTS 10

int fd_epc660_enable_dir;
int fd_epc660_enable_val;
int fd_5v_10v_enable_dir;
int fd_5v_10v_enable_val;
int fd_3v3_enable_dir;
int fd_3v3_enable_val;
int fd_m5v_enable_dir;
int fd_m5v_enable_val;
int fd_boot_disable_dir;
int fd_boot_disable_val;
int fd_bbb_epc660_shutter_dir;
int fd_bbb_epc660_shutter_val;
int fd_bbb_epc660_vsync_a0_dir;
int fd_bbb_epc660_vsync_a0_val;

/*!
 Exports all the needed GPIOs and sets them as an input or output
 @returns On success, 0, on error -1
 */
void bootInit() {
	/* Export GPIOs required for boot sequence: */
	gpio_export(BBB_EPC660_ENABLE);
	gpio_export(BBB_5V_10V_ENABLE);
	gpio_export(BBB_3V3_ENABLE);
	gpio_export(BBB_MINUS_5V_ENABLE);
	gpio_export(BBB_BOOT_DISABLE);
	gpio_export(BBB_EPC660_SHUTTER);
	gpio_export(BBB_LED_ENABLE);

	/* Open direction and value files: */
	fd_epc660_enable_dir = gpio_open_dir(BBB_EPC660_ENABLE);
	fd_epc660_enable_val = gpio_open_val(BBB_EPC660_ENABLE);

	fd_5v_10v_enable_dir = gpio_open_dir(BBB_5V_10V_ENABLE);
	fd_5v_10v_enable_val = gpio_open_val(BBB_5V_10V_ENABLE);

	fd_3v3_enable_dir = gpio_open_dir(BBB_3V3_ENABLE);
	fd_3v3_enable_val = gpio_open_val(BBB_3V3_ENABLE);

	fd_m5v_enable_dir = gpio_open_dir(BBB_MINUS_5V_ENABLE);
	fd_m5v_enable_val = gpio_open_val(BBB_MINUS_5V_ENABLE);

	fd_boot_disable_dir = gpio_open_dir(BBB_BOOT_DISABLE);
	fd_boot_disable_val = gpio_open_val(BBB_BOOT_DISABLE);

	fd_bbb_epc660_shutter_dir = gpio_open_dir(BBB_EPC660_SHUTTER);
	fd_bbb_epc660_shutter_val = gpio_open_val(BBB_EPC660_SHUTTER);

	gpio_set_dir(fd_boot_disable_dir, "out");
	gpio_set_val(fd_boot_disable_val, 1);
	gpio_set_dir(fd_epc660_enable_dir, "out");
	gpio_set_val(fd_epc660_enable_val, 0);
	gpio_set_dir(fd_5v_10v_enable_dir, "out");
	gpio_set_val(fd_5v_10v_enable_val, 0);
	gpio_set_dir(fd_m5v_enable_dir, "out");
	gpio_set_val(fd_m5v_enable_val, 0);
	gpio_set_dir(fd_3v3_enable_dir, "out");
	gpio_set_val(fd_3v3_enable_val, 0);
	gpio_set_dir(fd_bbb_epc660_shutter_dir, "in");
	gpio_set_val(fd_bbb_epc660_shutter_val, 0);
}

void bootRelease() {
	gpio_close_dir(fd_epc660_enable_dir);
	gpio_close_val(fd_epc660_enable_val);
	gpio_close_dir(fd_5v_10v_enable_dir);
	gpio_close_val(fd_5v_10v_enable_val);
	gpio_close_dir(fd_3v3_enable_dir);
	gpio_close_val(fd_3v3_enable_val);
	gpio_close_dir(fd_m5v_enable_dir);
	gpio_close_val(fd_m5v_enable_val);
	gpio_close_dir(fd_boot_disable_dir);
	gpio_close_val(fd_boot_disable_val);
	gpio_close_dir(fd_bbb_epc660_shutter_dir);
	gpio_close_val(fd_bbb_epc660_shutter_val);
	gpio_close_dir(fd_bbb_epc660_vsync_a0_dir);
	gpio_close_val(fd_bbb_epc660_vsync_a0_val);
}

int boot() {
	unsigned int count = 0;
	unsigned char *i2cData = malloc(4 * sizeof(unsigned char));
	unsigned int deviceAddress;

	while (count < N_BOOT_ATTEMPTS) {
		bootInit();
		usleep(50000); // wait for 50 ms
		gpio_set_val(fd_3v3_enable_val, 1);
		usleep(50000); // wait for 50 ms
		gpio_set_val(fd_5v_10v_enable_val, 1);
		usleep(50000); // wait for 50 ms
		gpio_set_val(fd_m5v_enable_val, 1);
		usleep(50000); // wait for 50 ms
		gpio_set_val(fd_epc660_enable_val, 1);
		usleep(500000); // wait for 500 ms
		i2c(0, 'd', 0, 0, &i2cData); // Scan for i2c devices
		deviceAddress = i2cData[0];
		printf("deviceAddress = 0x%x\n", i2cData[0]);
		printf("deviceAddress = 0x%x\n", i2cData[1]);
		printf("deviceAddress = 0x%x\n", i2cData[2]);
		printf("deviceAddress = 0x%x\n", i2cData[3]);

		if (deviceAddress >= 0x20 && deviceAddress <= 0x23){ // if device address correct
			count = N_BOOT_ATTEMPTS; // Leave while loop.
		}else if (count < N_BOOT_ATTEMPTS-1){
			bootUndo();
			bootRelease();
			bootUnexportAll();
		}else {
			bootUndo();
			bootRelease();
			bootUnexportAll();
			free(i2cData);
			return -1;
		}
		count++;
	}
	free(i2cData);

	// Read PART_TYPE:
	configSetPartType(bootGetPartType(deviceAddress));

	configSetPartVersion(bootGetPartVersion(deviceAddress));

	// Read IC_VERSION:
	configSetIcVersion(bootGetIcVersion(deviceAddress));

	// Read WAFER_ID:
	configSetWaferID(bootGetWaferID(deviceAddress));

	// Read CHIP_ID:
	configSetChipID(bootGetChipID(deviceAddress));

	configSetDeviceAddress(deviceAddress);

	bootNormalMode(deviceAddress);
	bootRelease();
	deviceAddress=0x22;
	return deviceAddress;
}

unsigned int bootGetIcVersion(unsigned int deviceAddress){
	unsigned char *i2cRes = malloc(1 * sizeof(unsigned char));
	i2c(deviceAddress, 'r', IC_VERSION, 1, &i2cRes);
	unsigned int icVersion = i2cRes[0];
	printf("IC_VERSION: %i\n", icVersion);
	free(i2cRes);
	return icVersion;
}

unsigned int bootGetPartType(unsigned int deviceAddress){
	unsigned char *i2cRes = malloc(1 * sizeof(unsigned char));
	i2cRes[0] = PART_TYPE;
	i2c(deviceAddress, 'w', EE_ADDR, 1, &i2cRes);
	i2c(deviceAddress, 'r', EE_DATA, 1, &i2cRes);
	unsigned int partType = i2cRes[0];
	printf("PART_TYPE: %i\n", partType);
	free(i2cRes);
	return partType;
}

unsigned int bootGetPartVersion(unsigned int deviceAddress){
	unsigned char *i2cRes = malloc(1 * sizeof(unsigned char));
	i2cRes[0] = PART_VERSION;
	i2c(deviceAddress, 'w', EE_ADDR, 1, &i2cRes);
	i2c(deviceAddress, 'r', EE_DATA, 1, &i2cRes);
	unsigned int partVersion = i2cRes[0];
	printf("PART_VERSION: %i\n", partVersion);
	free(i2cRes);
	return partVersion;
}


uint16_t bootGetWaferID(unsigned int deviceAddress){
	unsigned char *i2cRes = malloc(1 * sizeof(unsigned char));
	i2cRes[0] = WAFER_ID_MSB;
	i2c(deviceAddress, 'w', EE_ADDR, 1, &i2cRes);
	i2c(deviceAddress, 'r', EE_DATA, 1, &i2cRes);
	uint16_t waferID = i2cRes[0] << 8;
	i2c(deviceAddress, 'r', EE_DATA, 1, &i2cRes);
	waferID += i2cRes[0];
	printf("WAFER_ID: %i\n", waferID);
	free(i2cRes);
	return waferID;
}

uint16_t bootGetChipID(unsigned int deviceAddress){
	unsigned char *i2cRes = malloc(1 * sizeof(unsigned char));
	i2cRes[0] = CHIP_ID_MSB;
	i2c(deviceAddress, 'w', EE_ADDR, 1, &i2cRes);
	i2c(deviceAddress, 'r', EE_DATA, 1, &i2cRes);
	uint16_t chipID = i2cRes[0] << 8;
	i2c(deviceAddress, 'r', EE_DATA, 1, &i2cRes);
	chipID += i2cRes[0];
	printf("CHIP_ID: %i\n", chipID);
	free(i2cRes);
	return chipID;
}

/*!
 Unexports all the GPIOs.
 */
void bootUnexportAll() {
	gpio_unexport(BBB_EPC660_ENABLE);
	gpio_unexport(BBB_5V_10V_ENABLE);
	gpio_unexport(BBB_3V3_ENABLE);
	gpio_unexport(BBB_MINUS_5V_ENABLE);
	gpio_unexport(BBB_BOOT_DISABLE);
	gpio_unexport(BBB_EPC660_SHUTTER);
}

/*!
 Disables 10V, 5V, 3.3V and -10V.
 */
void bootUndo() {
	gpio_set_val(fd_boot_disable_val, 0);
	gpio_set_val(fd_epc660_enable_val, 0);
	gpio_set_val(fd_m5v_enable_val, 0);
	gpio_set_val(fd_5v_10v_enable_val, 0);
	gpio_set_val(fd_3v3_enable_val, 0);
	usleep(1000000); // wait a second
}

void bootNormalMode(unsigned int deviceAddress){
	unsigned char *i2cData = malloc(4 * sizeof(unsigned char));
	i2c(deviceAddress, 'r', CFG_MODE_STATUS, 1, &i2cData);
	if ((i2cData[0] & 0x80) == 0x80){
		printf("Changing mode...\n");
		printf("Enabling PLL...\n");
		i2cData[0] = 0x9F; // enable PLL, update EEPROM timers, copy EEPROM
		i2c(deviceAddress, 'w', CFG_MODE_CONTROL, 1, &i2cData);
		usleep(1000);

		i2cData[0] = 0x00; // Disable block clocks.
		i2c(deviceAddress, 'w', CLK_ENABLES, 1, &i2cData);

		printf("Disabling PLL bypass...\n");
		i2cData[0] = 0x94;	// PLL bypass disable
		i2c(deviceAddress, 'w', CFG_MODE_CONTROL, 1, &i2cData);

		printf("Disabling sys_clk bypass...\n");
		i2cData[0] = 0x84; // sys_clk bypass disable
		i2c(deviceAddress, 'w', CFG_MODE_CONTROL, 1, &i2cData);

		printf("Switching to normal mode...\n");
		i2cData[0] = 0x04; // Switch to normal mode.
		i2c(deviceAddress, 'w', CFG_MODE_CONTROL, 1, &i2cData);
		i2cData[0] = 0x3F; // Enable block clocks.
		i2c(deviceAddress, 'w', CLK_ENABLES, 1, &i2cData);
	}
	free(i2cData);
}

void bootReset(){
	/* Open direction and value files: */
	fd_epc660_enable_dir = gpio_open_dir(BBB_EPC660_ENABLE);
	fd_epc660_enable_val = gpio_open_val(BBB_EPC660_ENABLE);

	gpio_set_val(fd_epc660_enable_val, 0);
	usleep(1000); // wait for 100 us
	gpio_set_val(fd_epc660_enable_val, 1);
	usleep(500000); // wait for 500 ms

	//gpio_close_dir(fd_epc660_enable_dir);
	//gpio_close_val(fd_epc660_enable_val);

}


void bootCloseEpc660_enable(){
	gpio_close_dir(fd_epc660_enable_dir);
	gpio_close_val(fd_epc660_enable_val);
}


/// @}
