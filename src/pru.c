#define _GNU_SOURCE

#include "pru.h"
#include "registers.h"
#include "i2c.h"
#include "fp_atan2.h"
#include "api.h"
#include "helper.h"
#include "dll.h"
#include "temperature.h"
#include "gpio.h"
#include "evalkit_gpio.h"
#include <prussdrv.h>
#include <pruss_intc_mapping.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdint.h>
/// \addtogroup pru
/// @{

static unsigned int nRowsPerHalf;
static unsigned int nRowsPerHalfRaw;
static unsigned int nCols;
static unsigned int nColsRaw;
static unsigned int nDCS;
static unsigned int minAmplitude = 0;
static unsigned int rowReduction = 1;
static unsigned int colReduction = 1;

static int gStartFlag = 0;
static unsigned int numberOfColumnsMax            =  328;  /*! max value for a single row */
static unsigned int numberOfBytesPerHsyncPulseMax = 1312;  /*! max value for data during a single HSYNC pulse in bytes */
static unsigned int nHalves                       =    2;  /*! 2: two mirrored ROI halves; 1: ROI not mirrored */

static int fd_mem;
static unsigned int ddr_map_base_addr;
static unsigned ddr_map_size;
static tpruss_intc_initdata pruss_intc_initdata = PRUSS_INTC_INITDATA;
static uint16_t* pMem;
static uint32_t* pMem32;
static unsigned int* pPruConf;
static unsigned int* pPruCaptureOffset;
static unsigned int* pPruMemStart;
static unsigned int* pPruMemEnd;
static unsigned char* pData;

static unsigned int deviceAddress;
static uint8_t startFlag = 1;

/**
 Initializes the PRU and variables required to get a picture.
 @return On success, 0 is returned. On error, -1 is returned.
 */
int pruInit(const unsigned int addressOfDevice) {

	if(startFlag==1){
		startFlag=0;
	}else{
		pruRelease();
	}

	pData = malloc(1 * sizeof(unsigned char));
	pData[0] = 0x01;
	FILE* fd_map;
	deviceAddress = addressOfDevice;
	unsigned int str_len = 11;
	char line[str_len];

	if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
		numberOfColumnsMax            = 168;
		numberOfBytesPerHsyncPulseMax = 4 * numberOfColumnsMax; // 2 * 2 = 4 because 12 bit data is split into to parts.
		nRowsPerHalfRaw = 60;
		nRowsPerHalf    = nRowsPerHalfRaw;
		nColsRaw        = 160;
		nCols           = nColsRaw;
		nHalves         = 1;
	} else {
		numberOfColumnsMax            = 328;
		numberOfBytesPerHsyncPulseMax = 4 * numberOfColumnsMax;
		nRowsPerHalfRaw = 120;
		nRowsPerHalf    = nRowsPerHalfRaw;
		nColsRaw        = 320;
		nCols           = nColsRaw;
		nHalves         = 2;
	}

	if (configIsFLIM()) {
		nDCS = 2;
	} else {
		nDCS = 4;
	}

	/* Load start address and size of memory mapping: */
	fd_map = fopen("/sys/class/uio/uio0/maps/map1/addr", "r");
	if (fd_map == NULL) {
		perror("Failed to open a file for dumping data\n");
		return -1;
	}
	fgets(line, str_len, fd_map);
	fflush(stdin);
	fclose(fd_map);
	ddr_map_base_addr = (unsigned int) strtoll(line, NULL, 0);

	fd_map = fopen("/sys/class/uio/uio0/maps/map1/size", "r");
	if (fd_map == NULL) {
		perror("Failed to open a file for dumping data\n");
		return -1;
	}
	fgets(line, str_len, fd_map);
	fflush(stdin);
	fclose(fd_map);
	ddr_map_size = (unsigned int) strtoll(line, NULL, 0);

	if (ddr_map_size < REQUIRED_MEM_SIZE) {
		printf("Memory mapping is too small.\n");
		return -1;
	}
	//	/* Initialize the PRU: */
	prussdrv_init();

	//	/* Open PRU Interrupt: */
	prussdrv_open(PRU_EVTOUT_0);

	/* Open the file for the memory device: */
	fd_mem = open("/dev/mem", O_RDWR | O_SYNC); //slow
//			fd_mem = open("/dev/mem", O_RDWR); // horizontal bars in picture :(
	if (fd_mem < 0) {
		perror("\x1b[31m" "Failed to open /dev/mem (%s)\n" "\x1b[0m");
		return -1;
	}

	pMem = mmap(0, ddr_map_size, PROT_WRITE | PROT_READ, MAP_SHARED, fd_mem, ddr_map_base_addr);
	pMem32 = (uint32_t*)pMem;

	if ((pMem) == MAP_FAILED) {
		perror("\x1b[31m" "Failed to map the device\n." "\x1b[0m");
		close(fd_mem);
		return -1;
	}

	if (mlock(pMem, ddr_map_size) != 0) { // Makes sure that the used pages stay in the memory.
		perror("\x1b[31m" "mlock failed" "\x1b[0m");
		close(fd_mem);
		return -1;
	}

	/* Storing PRU configuration into PRU1 DRAM: */
	pPruConf = malloc(PRU_CONF_SIZE);
	pPruCaptureOffset = pPruConf;
	pPruMemStart      = pPruConf + 1;
	pPruMemEnd        = pPruConf + 2;

	*pPruCaptureOffset = (numberOfColumnsMax - nCols) * 2; // offset for assembler program
	*pPruMemStart      = ddr_map_base_addr; // start address for data
	*pPruMemEnd        = (*pPruMemStart) + nRowsPerHalf * nHalves * nCols * nDCS * 2; // last data address+1

	prussdrv_pru_write_memory(PRUSS0_PRU1_DATARAM, 0, pPruConf, PRU_CONF_SIZE);

	gStartFlag = 0;

	return 0;
}

/**
 Sets the size (columns x rows per half).
 @param nOfColumns the number of columns
 @param nOfRowsPerHalf the number of rows / 2
 */
void pruSetSize(const unsigned int nOfColumns, const unsigned int nOfRowsPerHalf) {
	nColsRaw        = nOfColumns;
	nCols           = nColsRaw / colReduction;
	nRowsPerHalfRaw = nOfRowsPerHalf;
	nRowsPerHalf    = nRowsPerHalfRaw / rowReduction;

	*pPruMemEnd        = (*pPruMemStart) + nRowsPerHalf * nHalves * nCols * nDCS * 2; // *2 to get number of bytes
	*pPruCaptureOffset = (numberOfColumnsMax - nCols) * 2;

	prussdrv_pru_write_memory(PRUSS0_PRU1_DATARAM, 0, pPruConf, PRU_CONF_SIZE);
}

int pruGetNumberOfHalves() {
	return nHalves;
}

/*!
 @brief Sets the column reduction
 @param reduction factor for reduction
 */
void pruSetColReduction(int reduction) {
	colReduction = pow(2, reduction);
	pruSetSize(nColsRaw, nRowsPerHalfRaw);
}

/*!
 Getter function for reduction
 @returns current column reduction
 */
int pruGetColReduction() {
	return colReduction;
}

/*!
 Sets the row reduction
 @param reduction factor for reduction
 */
void pruSetRowReduction(int reduction) {
	rowReduction = pow(2, reduction);
	pruSetSize(nColsRaw, nRowsPerHalfRaw);
}

/*!
 Getter function for reduction
 @returns current row reduction
 */
int pruGetRowReduction() {
	return rowReduction;
}

int pruGetNDCS(){
	return nDCS;
}

/*!
 Sets the number of DCS.
 @param nOfDCS the number of DCS
 */
void pruSetDCS(const unsigned int nOfDCS) {
	nDCS = nOfDCS;
	*pPruMemEnd = (*pPruMemStart) + nRowsPerHalf * nHalves * nCols * nDCS * 2;
	prussdrv_pru_write_memory(PRUSS0_PRU1_DATARAM, 0, pPruConf, PRU_CONF_SIZE);
}

/*!
 Sets the minimum value of the amplitude.
 @param minAmp minimum amplitude value
 */
void pruSetMinAmplitude(const unsigned int minAmp) {
	minAmplitude = minAmp;
}

/*!
 Gets the minimum value of the amplitude.
 @returns minAmplitude
 */
int pruGetMinAmplitude() {
	return minAmplitude;
}

/*!
 Getter function for nRowsPerHalf
 @returns current number of rows per half
 */
int pruGetNRowsPerHalf() {
	return nRowsPerHalf;
}

/*!
 Getter function for nCols
 @returns current number of columns
 */
int pruGetNCols() {
	return nCols;
}

/*!
 Executes code on PRU, sends shutter and waits for event.
 @param data pointer to the pointer pointing at the start address, where the data will be stored at
 @returns number of pixels
 */
int pruGetImage(uint16_t **data) {
	//double elapsedTime;
	//struct timeval tv1, tv2;
	//gettimeofday(&tv1, NULL);
    prussdrv_pruintc_init(&pruss_intc_initdata);

	if(gStartFlag == 0){
		gStartFlag = 1; //execute only one time
		if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
			prussdrv_exec_program(PRU_NUM, "./pru_capture_epc635.bin");
		} else {
			prussdrv_exec_program(PRU_NUM, "./pru_capture_epc660.bin");
		}
	} else {
		prussdrv_pru_send_event (ARM_PRU0_INTERRUPT + 16);
	}

	i2c(deviceAddress, 'w', SHUTTER_CONTROL, 1, &pData);
	prussdrv_pru_wait_event (PRU_EVTOUT_0);

	//gettimeofday(&tv2, NULL);
	//elapsedTime = (double)(tv2.tv_sec - tv1.tv_sec) + (double)(tv2.tv_usec - tv1.tv_usec)/1000000.0;
	//printf("seconds elapsed in ms = %2.4f\n", elapsedTime *1000.0);

	*data = pMem;
	int size = nRowsPerHalf * nHalves * nCols * nDCS;

	return size;		// number of pixels
}

void pruPrime(void) {

    prussdrv_pruintc_init(&pruss_intc_initdata);

	if(gStartFlag == 0){
		gStartFlag = 1; //execute only one time
		if (configGetPartType() == EPC635 || configGetPartType() == EPC503) {
			prussdrv_exec_program(PRU_NUM, "./pru_capture_epc635.bin");
		} else {
			prussdrv_exec_program(PRU_NUM, "./pru_capture_epc660.bin");
		}
	} else {
		prussdrv_pru_send_event (ARM_PRU0_INTERRUPT + 16);
	}

}

int pruCollect(uint16_t **data) {

	prussdrv_pru_wait_event (PRU_EVTOUT_0);

	*data = pMem;
	int size = nRowsPerHalf * nHalves * nCols * nDCS;

	return size;		// number of pixels
}

/*!
 Releases data used by the PRU
 @return On success, 0 is returned. On error, -1 is returned.
 */
int pruRelease() {	//free???
	free(pData);
	free(pPruConf);
	prussdrv_pru_disable(PRU_NUM);
	prussdrv_exit();
	if (munlock(pMem, ddr_map_size) != 0) {
		perror("\x1b[31m" "Unlocking memory failed" "\x1b[0m");
		return -1;
	}
	if (munmap(pMem, ddr_map_size) < 0) {
		perror("\x1b[31m" "Unmapping memory failed." "\x1b[0m");
		return -1;
	}
	if (close(fd_mem) != 0) {
		printf("\x1b[31m" "Closing fd_mem failed." "\x1b[0m");
		return -1;
	}
	return 0;
}

/// @}
