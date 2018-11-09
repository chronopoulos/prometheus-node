#ifndef GPIO_H_
#define GPIO_H_

/**************************************************************************
* Function Declarations                                                   *
**************************************************************************/

int gpio_export (unsigned int gpio);
int gpio_unexport (unsigned int gpio);

int gpio_open_dir (unsigned int gpio);
int gpio_set_dir (unsigned int fd_gpio_dir, const char* dir);
int gpio_get_dir (unsigned int fd_gpio_dir); // returns 0 for output, 1 for input
int gpio_close_dir (unsigned int fd_gpio_dir);

int gpio_open_val (unsigned int gpio);
int gpio_set_val (unsigned int fd_gpio_val, unsigned int val);
int gpio_get_val (unsigned int fd_gpio_val);
int gpio_close_val (unsigned int fd_gpio_val);

#endif
