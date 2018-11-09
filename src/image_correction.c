#include "image_correction.h"
#include "read_out.h"
#include "calculation.h"
#include "pru.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/// \addtogroup image_processing
/// @{

#define X 0
#define Y 1
#define W 0
#define NW 1
#define N 2
#define NE 3
#define E 4
#define SE 5
#define S 6
#define SW 7
#define CR 8
#define ST 9

//static unsigned int badPixels[1024][2];
pixelCorrection pixels[1024];
pixelCorrection pixelsTransformed[1024];
static unsigned int size;
//static unsigned int badPixelsPosition[1024];
static unsigned int counter;
static unsigned int imageCorrectionEnabled = 0;


void imageCorrectionLoad(const char *filename){
	FILE *file;
	file = fopen(filename, "r");
	size = 0;
	if (file!=NULL){
		char line[256];
		while (fgets(line, sizeof(line), file)) {
			char strX[3];
			char strY[3];
			char strN[2];
			strncpy(strX, line, 3);
			pixels[size].x = strtol(strX, NULL, 10);
			strncpy(strY, line + 4, 3);
			pixels[size].y = strtol(strY, NULL, 10);
			strncpy(strN, line + 8, 2);
			if (strN[1] == 13 || strN[1] == 10){
				strN[1] = '\0';
			}
			if (!strcmp("W",strN)){
				pixels[size].neighbour = W;
				printf("strN == W\n");
			}else if(!strcmp("NW",strN)){
				pixels[size].neighbour = NW;
			}else if(!strcmp("N",strN)){
				pixels[size].neighbour = N;
			}else if(!strcmp("NE",strN)){
				pixels[size].neighbour = NE;
			}
			else if(!strcmp("E",strN)){
				pixels[size].neighbour = E;
				printf("strN == E\n");
			}
			else if(!strcmp("SE",strN)){
				pixels[size].neighbour = SE;
			}
			else if(!strcmp("S",strN)){
				pixels[size].neighbour = S;
			}
			else if(!strcmp("SW",strN)){
				pixels[size].neighbour = SW;
			}
			size++;
		}
		fclose(file);
	}

}

int imageCorrectionEnable(const unsigned int state){
	imageCorrectionEnabled = state;
	return imageCorrectionEnabled;
}

void imageCorrectionTransform(){
	int xPos, yPos, i;
	int topLeftX = readOutGetTopLeftX();
	int bottomRightX = readOutGetBottomRightX();
	int topLeftY = readOutGetTopLeftY();
	int width = pruGetNCols();
	int yFactor = pruGetRowReduction();
	if (calculationIsDualMGX()){
		yFactor = 2;
	}
	counter = 0;
	for (i = 0; i < size; i++){
		if (pixels[i].x >= topLeftX && pixels[i].y <= bottomRightX){
			if (pixels[i].y >= topLeftY && pixels[i].y <= (251 - topLeftY)){
				if (pixels[i].x == topLeftX && (pixels[i].neighbour == SW  || pixels[i].neighbour == W || pixels[i].neighbour == NW)){
					printf("pixel %i,%i can not be corrected\n",pixels[i].x, pixels[i].y);
					continue;
				}
				if (pixels[i].x == bottomRightX && (pixels[i].neighbour == NE || pixels[i].neighbour == E || pixels[i].neighbour == SE)){
					printf("pixel %i,%i can not be corrected\n",pixels[i].x, pixels[i].y);
					continue;
				}
				if (pixels[i].y == topLeftY && (pixels[i].neighbour == NW || pixels[i].neighbour == N || pixels[i].neighbour == NE)){
					printf("pixel %i,%i can not be corrected\n",pixels[i].x, pixels[i].y);
					continue;
				}
				if (pixels[i].y == topLeftY && (pixels[i].neighbour == SW || pixels[i].neighbour == S || pixels[i].neighbour == SE)){
					printf("pixel %i,%i can not be corrected\n",pixels[i].x, pixels[i].y);
					continue;
				}
				xPos = (pixels[i].x - topLeftX) / pruGetColReduction();
				yPos = (pixels[i].y - topLeftY) / yFactor;
				pixels[i].pos = yPos * width + xPos;
				pixelsTransformed[counter] = pixels[i];
				counter++;
			}
		}
	}
}

void imageCorrect(uint16_t **data){
	if (imageCorrectionEnabled){
		int i;
		for (i = 0; i < counter; i++){
			imageCorrectCopyPixel(pixelsTransformed[i],data);
		}
	}
}

void imageCorrectCopyPixel(const pixelCorrection pixel, uint16_t **data){
	uint16_t *pMem = *data;
	int width = pruGetNCols();
	switch(pixel.neighbour){
	case W:
		pMem[pixel.pos] = pMem[pixel.pos - 1];
		break;
	case NW:
		pMem[pixel.pos] = pMem[pixel.pos - width - 1];
		break;
	case N:
		pMem[pixel.pos] = pMem[pixel.pos - width];
		break;
	case NE:
		pMem[pixel.pos] = pMem[pixel.pos - width + 1];
		break;
	case E:
		pMem[pixel.pos] = pMem[pixel.pos + 1];
		break;
	case SE:
		pMem[pixel.pos] = pMem[pixel.pos + width + 1];
		break;
	case S:
		pMem[pixel.pos] = pMem[pixel.pos + width];
		break;
	case SW:
		pMem[pixel.pos] = pMem[pixel.pos + width - 1];
		break;
	default:
		printf("unknown neighbour, check image_correction.txt\n");
		break;
	}
}


//too slow
void imageCorrectAveragePixel(const unsigned int pos, uint16_t **data){
	uint16_t *pMem = *data;
	uint16_t neighbourPixels[8];
	int width = pruGetNCols();
	neighbourPixels[0] = pMem[pos-1];
	neighbourPixels[1] = pMem[pos-width-1];
	neighbourPixels[2] = pMem[pos-width];
	neighbourPixels[3] = pMem[pos-width+1];
	neighbourPixels[4] = pMem[pos+1];
	neighbourPixels[5] = pMem[pos+width+1];
	neighbourPixels[6] = pMem[pos+width];
	neighbourPixels[7] = pMem[pos+width-1];

	printf("array filled\n");
	uint16_t t;
	uint8_t c, d;
	for (c = 1; c <= 8 - 1; c++){
		d = c;
		while (d > 0 && neighbourPixels[d] < neighbourPixels[d-1]){
			t = neighbourPixels[d];
			neighbourPixels[d] = neighbourPixels[d-1];
			neighbourPixels[d-1] = t;
			d--;
		}
	}
	for (c = 0; c < 8; c++){
		if (c == 3 || c == 4){
			printf("-->[%i] %i\n", c, neighbourPixels[c]);
		}
		else{
			printf("[%i] %i\n", c, neighbourPixels[c]);
		}
	}
	pMem[pos] = (neighbourPixels[3] + neighbourPixels[4]) / 2;
}


///@}
