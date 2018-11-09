#ifndef _I2C_H_
#define _I2C_H_

/**************************************************************************
* Function Declarations                                                   *
**************************************************************************/

int i2c (unsigned int dev_addr, char op, unsigned int reg_addr, unsigned int n_bytes_data, unsigned char** p_data);

#endif
