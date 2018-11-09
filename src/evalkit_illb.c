#include "evalkit_illb.h"
#include "evalkit_gpio.h"
#include "gpio.h"
#include <stdio.h>
#include <string.h>


/// \addtogroup illumination
/// @{

static int illbEnabled;

/*!
 Enables the illumination board.
 @returns On success optimal demodulation delay, on error -1
 */
int illuminationEnable() {
	/* Open direction and value files: */
	int fd_bbb_led_enable_dir = gpio_open_dir(BBB_LED_ENABLE);
	int fd_bbb_led_enable_val = gpio_open_val(BBB_LED_ENABLE);

	/* Set directions and values: */
	gpio_set_dir(fd_bbb_led_enable_dir, "out");
	gpio_set_val(fd_bbb_led_enable_val, 1);

	/* Close direction and value files:  */
	gpio_close_dir(fd_bbb_led_enable_dir);
	gpio_close_val(fd_bbb_led_enable_val);

	illbEnabled = 1;
	return 0;
}

/*!
 Disables the illumination board.
 @returns On success optimal demodulation delay, on error -1
 */
int illuminationDisable() {
	/* Open direction and value files: */
	int fd_bbb_led_enable_dir = gpio_open_dir(BBB_LED_ENABLE);
	int fd_bbb_led_enable_val = gpio_open_val(BBB_LED_ENABLE);

	/* Set directions and values: */
	gpio_set_dir(fd_bbb_led_enable_dir, "out");
	gpio_set_val(fd_bbb_led_enable_val, 0);

	/* Close direction and value files:  */
	gpio_close_dir(fd_bbb_led_enable_dir);
	gpio_close_val(fd_bbb_led_enable_val);

	illbEnabled = 0;
	return 0;
}

/*!
 Getter function for the illumination board
 @returns 0 if illumination board is enabled or 0 if it is disabled.
 */
int isIlluminationEnabled(){
	return illbEnabled;
}


/// @}

