
#define _XOPEN_SOURCE 500

#include "gpio.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdio.h>
#include <string.h>

/// \addtogroup gpio
/// @{


/**
 * Exports the control of a GPIO from kernel to user space.
 */
int gpio_export ( unsigned int gpio) {
  
  int fd = open("/sys/class/gpio/export", O_WRONLY);
  if ( fd < 0 ) {
    return -1;
  }
  char gpio_str[3];
  sprintf(gpio_str, "%i", gpio);

  if ( write(fd, gpio_str, strlen(gpio_str)) < 0) {
    return -2;
  }
  
  if ( close(fd) < 0 ) {
    return -3;
  }
  
  return 0;
}

/**
 * Returns the control of a GPIO from user to kernel space.
 */
int gpio_unexport ( unsigned int gpio) {
  
  int fd = open("/sys/class/gpio/unexport", O_WRONLY);
  if ( fd < 0 ) {
    return -1;
  }
  char gpio_str[3];
  sprintf(gpio_str, "%i", gpio);

  if ( write(fd, gpio_str, strlen(gpio_str)) < 0) {
    return -2;
  }
  
  if ( close(fd) < 0 ) {
    return -3;
  }
  
  return 0;
}

/**
 * Opens the direction file corresponding to the given GPIO.
 */
int gpio_open_dir (unsigned int gpio) {
  char path_to_dir[33];
  int fd_dir;

  sprintf(path_to_dir, "/sys/class/gpio/gpio%u/direction", gpio);

  fd_dir = open(path_to_dir, O_RDWR);
  if (fd_dir < 0) {
    return -1;
  }
  
  return fd_dir;
}

/**
 * Sets the direction (in, out) for the given GPIO.
 */
int gpio_set_dir (unsigned int fd_gpio_dir, const char* dir) {
  if ( write(fd_gpio_dir, dir , 3) < 0) {
    return -1;
  }
  return 0;
}

/**
 * Returns the direction (1: in, 0: out) for the given GPIO.
 */
int gpio_get_dir (unsigned int fd_gpio_dir) {
  char dir[1];
  if ( pread( fd_gpio_dir, dir, 1, 0) < 0 ) {
    return -1;
  }
  if (dir[0] == 'o') {      // output
    return 0;
  }
  else if (dir[0] == 'i') { // input
    return 1;
  }
  else {
    return -1;
  }
}

/**
 * Closes the direction file corresponding to the given GPIO.
 */
int gpio_close_dir (unsigned int fd_gpio_dir) {
  return close(fd_gpio_dir);
}

/**
 * Opens the value file corresponding to the given GPIO.
 */
int gpio_open_val (unsigned int gpio) {
  char path_to_val[29];
  int fd_val;

  sprintf(path_to_val, "/sys/class/gpio/gpio%u/value", gpio);

  fd_val = open(path_to_val, O_RDWR);
  if (fd_val < 0) {
    return -1;
  }
  
  return fd_val;
}

/**
 * Sets the value (0,1) for the given GPIO.
 */
int gpio_set_val (unsigned int fd_gpio_val, unsigned int val) {
  if (val) {
    if ( write(fd_gpio_val, "1" , 1) < 0 ) {
      return -1;
    }
  }
  else {
    if ( write(fd_gpio_val, "0" , 1) < 0 ) {
      return -1;
    }
  }
  return 0;
}

/**
 * Returns the value (1 or 0) for the given GPIO.
 */
int gpio_get_val (unsigned int fd_gpio_val) {
  char val[1];
  if ( pread( fd_gpio_val, val, 1, 0) < 0 ) {
    return -1;
  }
  if (val[0] == '0') {
    return 0;
  }
  else if (val[0] == '1') {
    return 1;
  }
  else {
    return -1;
  }
}

/**
 * Closes the value file corresponding to the given GPIO.
 */
int gpio_close_val (unsigned int fd_gpio_val) {
  return close(fd_gpio_val);
}

/// @}
