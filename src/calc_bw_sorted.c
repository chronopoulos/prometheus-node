#include "calc_bw_sorted.h"
#include "evalkit_constants.h"
#include "calculation.h"
#include "calculator.h"
#include "pru.h"
#include "config.h"
#include "saturation.h"
#include "adc_overflow.h"
#include <math.h>

#include <sys/time.h>
#include <string.h>
#include <time.h>
#include <unistd.h>


/// \addtogroup calculate
/// @{


static uint16_t pixelData[MAX_NUM_PIX * 5]; // max. size because 4DCS + 1 Grayscale image
static uint16_t pMem[4*MAX_NUM_PIX];
static int size=0;

/*!
Does grayscale/DCS image acquisition and returns sorted data array
@param data pointer to data pointer array. This arrays contains image data
@returns image size (number of pixels)
 */
int calcBWSorted(uint16_t **data) {
	size = pruGetImage(data);

	memcpy( (void*)pMem, (void*)(*data), size*sizeof(uint16_t));

	int nDCS = pruGetNDCS();
	int nPixelPerDCS = size / nDCS;
	int i, d;
	int x = 0;
	int y = 0;
	int posX, posY, pos;
	int width  = pruGetNCols();
	int height = 0;
	uint16_t adcMin    = adcOverflowGetMin();
	uint16_t adcMax    = adcOverflowGetMax();
	int adcOverflow    = adcOverflowIsEnabled();
	uint16_t pixelMask = calculatorGetPixelMask();

	if (configGetPartType() == EPC635 || configGetPartType() == EPC503 ) { // if statement before loop by purpose: faster
		uint16_t value;
		height = pruGetNRowsPerHalf();
		if (configGetCurrentMode() == FLIM){ //eps635 FLIM
			for (i = 0; i < size / 2; i++) {
				posX = x;
				posY = height - 1 - y;
				pos = posY * width + posX;
				value = pMem[i];
				saturationSet1DCS(value);
				if (saturationCheck()){
					pixelData[pos] = SATURATION;
				}else{
					pixelData[pos] = (value >> 4) - 2048;
				}
				value = pMem[i + size / 2];
				saturationSet1DCS(value);
				if (saturationCheck()){
					pixelData[pos + size / 2] = SATURATION;
				}else{
					pixelData[pos + size / 2] = (value >> 4) - 2048;
				}
				x++;
				if (x == width) {
					x = 0;
					y++;
				}
			}
		}else{ // epc635 grayscale mode
			for (d = 0; d < nDCS; d++){
				x=0;
				y=0;
				for (i = 0; i < nPixelPerDCS; i++) {
					posX  = x;
					posY  = height - 1 - y;
					pos   = posY * width + posX;
					value = pMem[i+d*nPixelPerDCS];
					saturationSet1DCS(value);
					if (saturationCheck()){
						pixelData[pos+d*nPixelPerDCS] = SATURATION;
					}else if ((value < adcMin || value > adcMax) && adcOverflow == 1){
						pixelData[pos+d*nPixelPerDCS] = ADC_OVERFLOW;
					}else{
						pixelData[pos+d*nPixelPerDCS] = value >> 4;
					}
					x++;
					if (x == width) {
						x = 0;
						y++;
					}
				}
			}
		}
	} else { // epc660 or epc502)
		int16_t value;
		height = pruGetNRowsPerHalf() * 2;
		if (configGetCurrentMode() == FLIM){	//FLIM mode
			for (i = 0; i < size / 2; i++) {
				if (x < width){
					posX =  x;
					posY = height / 2 - 1 - y;
				}else{
					posX = x - width;
					posY = height / 2 + y;
				}
				pos = posY * width + posX;
				value = pMem[i];
				saturationSet1DCS(value);
				if (saturationCheck()){
					pixelData[pos] = SATURATION;
				}else if ((value < adcMin || value > adcMax) && adcOverflow == 1){
					pixelData[pos] = ADC_OVERFLOW;
				}else{
					value = (value & pixelMask) - 2048;
					if (value < 0){
						pixelData[pos] = 0;
					}else{
						pixelData[pos] = value;
					}
				}
				value = pMem[i + size / 2];
				saturationSet1DCS(value);
				if (saturationCheck()){
					pixelData[pos + size / 2] = SATURATION;
				}else if ((value < adcMin || value > adcMax) && adcOverflow == 1){
					pixelData[pos + size / 2] = ADC_OVERFLOW;
				}else{
					value = (-(((int16_t)(value & pixelMask)) - 2048));
					if (value < 0){
						pixelData[pos + size / 2] = 0;
					}else{
						pixelData[pos + size / 2] = value;
					}
				}
				x++;
				if (x == 2 * width) {
					x = 0;
					y++;
				}
			}
		}else{ // epc660 grayscale mode
			for (d = 0; d < nDCS; d++){
				x=0;
				y=0;
				for (i = 0; i < nPixelPerDCS; i++) {
					if (x < width){
						posX =  x;
						posY = height / 2 - 1 - y;
					}else{
						posX = x - width;
						posY = height / 2 + y;
					}
					pos = posY * width + posX;
					value = pMem[i+d*nPixelPerDCS];
					saturationSet1DCS(value);
					value &= pixelMask;
					if (saturationCheck()){
						pixelData[pos+d*nPixelPerDCS] = SATURATION;
					}else if ((value < adcMin || value > adcMax) && adcOverflow == 1){
						pixelData[pos+d*nPixelPerDCS] = ADC_OVERFLOW;
					}else{
						pixelData[pos+d*nPixelPerDCS] = value;
					}
					x++;
					if (x == 2 * width) {
						x = 0;
						y++;
					}
				}	//end for i
			}
		}
	}
	*data = pixelData;

	return size;
}

void BWSortedAttachGrayscale(uint16_t* grayscaleAmp, int sizeGrayscaleAmp){
	for (int i = 0; i < sizeGrayscaleAmp; i++){
		pixelData[i+size]=grayscaleAmp[i];

	}
}

/// @}
