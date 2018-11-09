#include "pn.h"
#include "registers.h"
#include "i2c.h"
#include <stdio.h>
#include <stdlib.h>


/// \addtogroup pn
/// @{

static unsigned int polynomials[8][5] = {
	{0x8003, 0x8011, 0x8017, 0x802D, 0xFFFD},
	{0x402B, 0x4039, 0x4053, 0x405F, 0x7FE7},
	{0x201B, 0x2027, 0x2035, 0x2053, 0x3FFD},
	{0x1053, 0x1069, 0x107B, 0x107D, 0x1FC9},
	{0x0805, 0x0817, 0x082B, 0x082D, 0x0FE9},
	{0x0409, 0x041B, 0x0427, 0x042D, 0x07F9},
	{0x0211, 0x021B, 0x0221, 0x022D, 0x03FB},
	{0x011D, 0x012B, 0x012D, 0x014D, 0x01F5}
};

/*!
Selects the primitive polynomial for PN mode
@params indexLFSR
@params indexPolynomial
@param deviceAddress
returns selected polynomial
 */
int pnSelectPolynomial(unsigned int indexLFSR, unsigned int indexPolynomial, const int deviceAddress){
	if (indexLFSR < 1){
		indexLFSR = 1;
	}
	else if( indexLFSR > 7){
		indexLFSR = 7;
	}
	if(indexPolynomial > 4){
		indexPolynomial = 4;
	}
	unsigned char *i2cData = malloc(1 * sizeof(unsigned char));
	i2c(deviceAddress, 'r', MOD_CONTROL, 1, &i2cData);
	i2cData[0] =  (i2cData[0] & 0xF8) | indexLFSR;
	i2c(deviceAddress, 'w', MOD_CONTROL, 1, &i2cData);

	i2cData[0] = (polynomials[indexLFSR][indexPolynomial] >> 8) & 0xFF;
	i2cData[1] = polynomials[indexLFSR][indexPolynomial] & 0xFF;
	printf("i2cData[0] = %X\n", i2cData[0]);
	printf("i2cData[1] = %X\n", i2cData[1]);
	i2c(deviceAddress, 'w', PN_POLY_HI, 2, &i2cData);
	free(i2cData);
	return polynomials[indexLFSR][indexPolynomial];
}

/// @}
