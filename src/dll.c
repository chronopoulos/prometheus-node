#define _XOPEN_SOURCE 500

#include "dll.h"
#include "registers.h"
#include "i2c.h"
#include "evalkit_illb.h"
#include "config.h"
#include "pru.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>


/// \addtogroup DLL
/// @{

static unsigned char *i2cData;
static uint8_t initializeDLL;
static uint8_t dll_coarse_ctrl_ext_avg_prev;
static uint16_t dll_fine_ctrl_ext_avg_prev;

/*!
 Enables or disables the preheat
 @param state 1 for enable, 0 for disable
 @param deviceAddress i2c address of the chip (default 0x20)
 @returns 0 on success, -1 on error
 */
int dllEnablePreheat(const int state, const int deviceAddress) {
	unsigned char *i2cData = malloc(1 * sizeof(unsigned char));
	i2c(deviceAddress, 'r', LED_DRIVER, 1, &i2cData);
	int ledDriver;
	if (state) {
		ledDriver = i2cData[0] | 0x08;
	} else {
		ledDriver = i2cData[0] & 0xF7;
	}
	i2cData[0] = ledDriver;
	i2c(deviceAddress, 'w', LED_DRIVER, 1, &i2cData);
	free(i2cData);
	return 0;
}

/*!
 Enables or disables the bypassing of the DLL
 @param state 1 for enable, 0 for disable
 @param deviceAddress i2c address of the chip (default 0x20)
 @returns 0 on success, -1 on error
 */
int dllBypassDLL(const int state, const int deviceAddress) {
	unsigned char *i2cData = malloc(1 * sizeof(unsigned char));
	i2c(deviceAddress, 'r', DLL_CONTROL, 1, &i2cData);
	int dllControl;
	if (state) {
		dllControl = i2cData[0] | 0x01;
	} else {
		dllControl = i2cData[0] & 0xFE;
	}
	i2cData[0] = dllControl;
	i2c(deviceAddress, 'w', DLL_CONTROL, 1, &i2cData);
	if (!state){	//init dll if not bypassed
		dllInitialize(deviceAddress);
	}
	free(i2cData);
	return 0;
}

/*!
 Locks DLL depeding on the current mode (sinusoidal or PN) and the current
 modulation frequency by switching to the correct DLL synchronization
 frequency, i.e. equal to the LED modulation frequency during integration.
 The locking process is automatically done by the chip.
 @param deviceAddress i2c address of the chip (default 0x20)
 @returns 0 on success, -1 on error.
 */
int dllLockAutomatically(const int deviceAddress) {

	unsigned char* i2cData = malloc(6 * sizeof(unsigned char));
	unsigned char* integrationTimeSettings = malloc(4 * sizeof(unsigned char));
	unsigned char* roi_tl_y = malloc(1 * sizeof(unsigned char));
	unsigned short int mod_clk_divider = 0x00;
	unsigned short int demodulation_delays = 0x02;
	unsigned short int mod_control_prev;
	unsigned short int mod_clk_divider_prev;
	unsigned short int mode;
	unsigned int t_sync;
	unsigned short int n_locks = 0;

	uint8_t dll_coarse_ctrl_ext[N_LOCKS_REQ];
	uint16_t dll_fine_ctrl_ext[N_LOCKS_REQ];
	uint32_t dll_coarse_ctrl_ext_avg = 0;
	uint32_t dll_fine_ctrl_ext_avg = 0;
	uint8_t dll_fine_low_bank = 0;
	uint8_t dll_fine_bank = 0;
	int8_t i, n;

	unsigned int loopCount = 1;
	uint16_t loopCountMax = 100;

	// Determine mode:
	i2c(deviceAddress, 'r', MOD_CONTROL, 1, &i2cData);
	mod_control_prev = i2cData[0];
	printf("1\n");

	if ((mod_control_prev & 0xC0) == 0x00) {
		mode = 0; // sinusoidal mode
	} else if ((mod_control_prev & 0xC0) == 0x40) {
		mode = 1; // PN mode
	} else {
		return -1;
	}

	// Reduce number of DCSs:
	i2cData[0] = mod_control_prev & 0xCF;
	i2c(deviceAddress, 'w', MOD_CONTROL, 1, &i2cData);
	printf("2\n");

	// Store integration time settings:
	i2c(deviceAddress, 'r', INTM_HI, 4, &integrationTimeSettings);
	printf("3\n");

	// Set integration time to minimum:
	i2cData[0] = 0x00;
	i2cData[1] = 0x01;
	i2cData[2] = 0x00;
	i2cData[3] = 0x00;
	i2c(deviceAddress, 'w', INTM_HI, 4, &i2cData);
	printf("4\n");

	// Reduce ROI, i.e. only number of rows:
	i2c(deviceAddress, 'r', ROI_TL_Y, 1, &roi_tl_y);
	i2cData[0] = 0x76; // Makes sure that it also works in case of row reduction.
	i2c(deviceAddress, 'w', ROI_TL_Y, 1, &i2cData);
	printf("5\n");

	// Determine current mod_clk_divider:
	i2c(deviceAddress, 'r', MOD_CLK_DIVIDER, 1, &i2cData);
	mod_clk_divider_prev = i2cData[0];
	printf("6\n");

	if (mode == 0) { // Sinusoidal modulation
		if (mod_clk_divider_prev == 0x00) {
			mod_clk_divider = 1; // go down to 10 MHz
			// Set demodulation delay register:
			i2c(deviceAddress, 'r', DEMODULATION_DELAYS, 1, &i2cData);
			demodulation_delays = i2cData[0] | 0x10; // quadruples LED modulation frequency
			i2cData[0] = demodulation_delays;
			i2c(deviceAddress, 'w', DEMODULATION_DELAYS, 1, &i2cData);
		} else if (mod_clk_divider_prev == 0x01) {
			mod_clk_divider = 0x00; // go up to 20 MHz
		} else if (mod_clk_divider_prev == 0x03) {
			mod_clk_divider = 0x01; // go up to 10 MHz
		} else if (mod_clk_divider_prev == 0x07) {
			mod_clk_divider = 0x03; // go up to 5 MHz
		} else if (mod_clk_divider_prev == 0x0F) {
			mod_clk_divider = 0x07; // go up to 2.5 MHz
		} else if (mod_clk_divider_prev == 0x1F) {
			mod_clk_divider = 0x0F; // go up to 1.25 MHz
		} else {
			return -1;
		}
		// Change modulation frequency for synchronization phase:
		i2cData[0] = mod_clk_divider;
		i2c(deviceAddress, 'w', MOD_CLK_DIVIDER, 1, &i2cData);
	}
	printf("7\n");

	// Set length of DLL synchronization and enable DLL synchronization:
	i2cData[0] = 0x18;
	i2cData[1] = 0xFF;
	i2cData[2] = 0x25;
	i2cData[3] = 0x7F;
	i2cData[4] = 0x00;
	i2cData[5] = 0x01;
	i2c(deviceAddress, 'w', DLL_EN_DEL_HI, 6, &i2cData);
	printf("8\n");

	// Disable external delay line control:
	i2cData[0] = 0x00;
	i2c(deviceAddress, 'w', DLL_CONTROL, 1, &i2cData);
	printf("9\n");

	// Lock DLL - Repeat some times and get average.
	if (mod_clk_divider_prev == 0x00) {
		t_sync = 205;
	} else if (mod_clk_divider_prev == 0x01) {
		t_sync = 405;
	} else if (mod_clk_divider_prev == 0x03) {
		t_sync = 805;
	} else if (mod_clk_divider_prev == 0x07) {
		t_sync = 1605;
	} else if (mod_clk_divider_prev == 0x0F) {
		t_sync = 3205;
	} else if (mod_clk_divider_prev == 0x1F) {
		t_sync = 6405;
	} else {
		return -1;
	}

	//	printf("----------------------------\n");
	while (n_locks < N_LOCKS_REQ && loopCount <= loopCountMax) {
		// Send shutter to trigger DLL synchronization:
		i2cData[0] = 0x01;
		i2c(deviceAddress, 'w', SHUTTER_CONTROL, 1, &i2cData);
		printf("10\n");
		// Wait until locking procedure is finished.
		usleep(t_sync * 10);
		// Check whether DLL is locked and matched:
		i2c(deviceAddress, 'r', DLL_STATUS, 1, &i2cData);
		printf("11\n");

		if ((i2cData[0] & 0x01) && (i2cData[0] & 0x02)) { // DLL locked and matched
			i2c(deviceAddress, 'r', DLL_FINE_LOW_BANK_RB_HI, 6, &i2cData);
//			printf("0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X\n", i2cData[0], i2cData[1], i2cData[2], i2cData[3], i2cData[4], i2cData[5]);
			for (i = 15; i >= 0; i--) {
				if ((i2cData[0] << 8) + i2cData[1] < (1 << i)) {
					dll_fine_bank = i;
				}
				if ((i2cData[2] << 8) + i2cData[3] < (1 << i)) {
					dll_fine_low_bank = i;
				}
			}
			dll_fine_ctrl_ext[n_locks] = (i2cData[4] << 8) + (dll_fine_bank << 4) + dll_fine_low_bank;
			dll_coarse_ctrl_ext[n_locks] = i2cData[5];
			printf("dll_fine_ctrl_ext: 0x%04X (%d)\ndll_coarse_ctrl_ext: 0x%02X (%d)\n", dll_fine_ctrl_ext[n_locks], dll_fine_ctrl_ext[n_locks], dll_coarse_ctrl_ext[n_locks],
					dll_coarse_ctrl_ext[n_locks]);
			n_locks += 1;
			printf("INFO: DLL locked and matched %d times after a total of %d attempt(s).\n", n_locks, loopCount);
		} else if (loopCount == loopCountMax) {
			printf("WARNING: DLL locking failed (max. no. of loops reached).\n");
		}
		loopCount++;
	}
	//	printf("----------------------------\n");

	// Freeze DLL settings:
	i2cData[0] = 0x00;
	i2cData[1] = 0x00;
	i2c(deviceAddress, 'w', DLL_MEASUREMENT_RATE_HI, 2, &i2cData);
	printf("12\n");

	// Set average DLL configuration:
	for (i = 0; i < n_locks; i++) {
		dll_coarse_ctrl_ext_avg += dll_coarse_ctrl_ext[i];
	}
	dll_coarse_ctrl_ext_avg = (double) dll_coarse_ctrl_ext_avg / N_LOCKS_REQ + 0.5;

	n = 0;
	for (i = 0; i < n_locks; i++) {
		if (dll_coarse_ctrl_ext[i] == dll_coarse_ctrl_ext_avg) {
			dll_fine_ctrl_ext_avg += dll_fine_ctrl_ext[i];
			n++;
		}
	}
	dll_fine_ctrl_ext_avg = (double) dll_fine_ctrl_ext_avg / n + 0.5;

	for (i = 0; i < n_locks; i++) {
		if (dll_coarse_ctrl_ext[i] < dll_coarse_ctrl_ext_avg) {
			printf("INFO: Coarse value below average. Difference to average for fine value: %d\n", dll_fine_ctrl_ext[i] - dll_fine_ctrl_ext_avg);
		} else if (dll_coarse_ctrl_ext[i] > dll_coarse_ctrl_ext_avg) {
			printf("INFO: Coarse value above average. Difference to average for fine value: %d\n", dll_fine_ctrl_ext[i] - dll_fine_ctrl_ext_avg);
		}
	}

	if (initializeDLL || dll_coarse_ctrl_ext_avg != dll_coarse_ctrl_ext_avg_prev) {
		dll_fine_ctrl_ext_avg_prev = dll_fine_ctrl_ext_avg;
	} else {
		dll_fine_ctrl_ext_avg_prev = (dll_fine_ctrl_ext_avg_prev * WEIGHT_PREV + dll_fine_ctrl_ext_avg) / (1.0 + WEIGHT_PREV) + 0.5;
	}
	dll_coarse_ctrl_ext_avg_prev = dll_coarse_ctrl_ext_avg;

	// Apply average delay line settings (external control):
//	printf("###########################################\nApplied average value:\ndll_fine_ctrl_ext: 0x%04X (%d)\ndll_coarse_ctrl_ext: 0x%02X (%d)\n###########################################\n",
//			dll_fine_ctrl_ext_avg_prev, dll_fine_ctrl_ext_avg_prev, dll_coarse_ctrl_ext_avg_prev, dll_coarse_ctrl_ext_avg_prev);
	i2cData[0] = (dll_fine_ctrl_ext_avg_prev >> 8) & 0x03;
	i2cData[1] = dll_fine_ctrl_ext_avg_prev & 0xFF;
	i2cData[2] = dll_coarse_ctrl_ext_avg_prev;
	i2c(deviceAddress, 'w', DLL_FINE_CTRL_EXT_HI, 3, &i2cData);

	// Enable external delay line control:
	i2cData[0] = 0x04;
	i2c(deviceAddress, 'w', DLL_CONTROL, 1, &i2cData);

	// Restore integration time settings:
	i2c(deviceAddress, 'w', INTM_HI, 4, &integrationTimeSettings);

	// Restore ROI:
	i2c(deviceAddress, 'w', ROI_TL_Y, 1, &roi_tl_y);

	// Restore modulation frequency:
	if (mode == 0) {
		i2cData[0] = mod_clk_divider_prev;
		i2c(deviceAddress, 'w', MOD_CLK_DIVIDER, 1, &i2cData);
		if (mod_clk_divider_prev == 0x00) {
			i2cData[0] = (demodulation_delays & 0xEF);
			i2c(deviceAddress, 'w', DEMODULATION_DELAYS, 1, &i2cData);
		}
	}

	// Restore number of DCSs:
	i2cData[0] = mod_control_prev;
	i2c(deviceAddress, 'w', MOD_CONTROL, 1, &i2cData);

	free(roi_tl_y);
	free(i2cData);
	free(integrationTimeSettings);

	return 0;
}


/*!
 Initializes the DLL.
 @param deviceAddress i2c address of the chip (default 0x20)
 @return 0 on success, -1 on error.
 */
int dllInitialize(const int deviceAddress) {
	i2cData = malloc(4 * sizeof(unsigned char));
	uint8_t mod_clk_divider;
	uint8_t dll_clk_divider;

	initializeDLL = 1;

	// Determine current mod_clk_divider:
	i2c(deviceAddress, 'r', MOD_CLK_DIVIDER, 1, &i2cData);
	mod_clk_divider = i2cData[0];

	// Change DLL frequency:
	if (mod_clk_divider == 0x00) {
		dll_clk_divider = 0x03;
	} else if (mod_clk_divider == 0x01) {
		dll_clk_divider = 0x07;
	} else if (mod_clk_divider == 0x03) {
		dll_clk_divider = 0x0F;
	} else if (mod_clk_divider == 0x07) {
		dll_clk_divider = 0x1F;
	} else if (mod_clk_divider == 0x0F) {
		dll_clk_divider = 0x3F;
	} else if (mod_clk_divider == 0x1F) {
		dll_clk_divider = 0x7F;
	} else {
		return -1;
	}
	i2cData[0] = dll_clk_divider;
	i2c(deviceAddress, 'w', DLL_CLK_DIVIDER, 1, &i2cData);

//	dllDetermineAndSetDemodulationDelay(deviceAddress); // required only once during start-up

	// Configure DLL jitter filters:
	if (mod_clk_divider == 0) { // 20 MHz
		i2cData[0] = 0x73;
	} else {
		// ### TODO: Optimal values need to be determined. ###
		i2cData[0] = 0x77;
	}
	i2c(deviceAddress, 'w', DLL_FILTER, 1, &i2cData);

	// Initialize delay line:
	dllLockAutomatically(deviceAddress);

	free(i2cData);

	initializeDLL = 0;
	return 0;
}

/*!
 Determines and sets the optimum demodulation delay.
 @param deviceAddress i2c address of the chip (default 0x20)
 @return Optimal demodulation delay on success, -1 on error.
 */
int dllDetermineAndSetDemodulationDelay(const int deviceAddress) {
    i2cData = malloc(1 * sizeof(unsigned char));

	i2cData[0] = 0x02;
	i2c(deviceAddress, 'w', DEMODULATION_DELAYS, 1, &i2cData);

	i2cData[0] = 0x01;
	i2c(deviceAddress, 'w', DLL_CONTROL, 1, &i2cData);

	free(i2cData);

	return 2;
}

int dllGetCoarseCtrlExt() {
    return dll_coarse_ctrl_ext_avg_prev;
}

int dllGetFineCtrlExt() {
    return dll_fine_ctrl_ext_avg_prev;
}

void dllResetDemodulationDelay(const int deviceAddress){
    i2cData = malloc(1 * sizeof(unsigned char));

	i2cData[0] = 0x00;
	i2c(deviceAddress, 'w', DEMODULATION_DELAYS, 1, &i2cData);
	free(i2cData);
}


/// @}
