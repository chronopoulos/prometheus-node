#ifndef _CALCULATION_H_
#define _CALCULATION_H_

#include <stdint.h>

#include "evalkit_constants.h"

enum calculationType {
	DIST, AMP, INFO
} ;



int16_t setEnableImaging(const int status);

int calculationBWSorted(uint16_t **data);
int calculationDCSSorted(uint16_t **data);
int getDCSTOFeAndGrayscaleSorted(uint16_t **data, int nDCSTOF_log);
int calculationDistanceSorted(uint16_t **data);
int calculationAmplitudeSorted(uint16_t **data);
int calculationDistanceAndAmplitudeSorted(uint16_t **data);
void calculationEnablePiDelay(const int state);
void calculationEnableDualMGX(const int state);
void calculationEnableHDR(const int state);
void calculationEnableLinear(const int state);
void calculationEnablePN(const int state);
int calculationIsPiDelay();
int calculationIsDualMGX();
int calculationIsHDR();
int calculationIsPN();
int calculationBW(int16_t **data);

void setCorrectGrayscaleGain(int enable);
void setCorrectGrayscaleOffset(int enable);
void calculationGrayscaleMean(int numAveragingImages, int* data);
int  calculationCalibrateGrayscale(int mode);
void calculationGrayscaleGainOffset();
void calculateGrayscaleCorrection(uint16_t* data);

int calculateSaveGrayscaleGainOffset();
int calculateLoadGrayscaleGainOffset();
int calculateLoadGrayscaleImage(int *data);
int calculateSaveGrayscaleImage(int *data);

void calculationSetEnableDRNU(int enable);
int  calculationGetEnableDRNU();
void calculationSetEnableTemperature(int enable);
int  calculationGetEnableTemperature();
void calculationSetEnableAmbientLight(int enable);
int calculationGetEnableAmbientLight();

int  calculationInterpolate(double x, double x0, double y0, double x1, double step);
int  calculationTemperatureCompensation(uint16_t** data);
int  calculationCorrectDRNU(uint16_t** data);
int  calculationCorrectDRNU2(uint16_t** data);
void calculationAddOffset(uint16_t **data);
void calculationAmbientDCSCorrection(uint16_t **data, int pixelPerDCS);
int  calculationAmbientDCSCorrectionPixel(uint16_t pixel, uint16_t ampGrayscale);
int  calculationSetAmbientLightFactor(double factor);
void calculationSetAmbientLightFactorOrg(double factor);
void calculationSetDiffTemp(int tempDiff);
double  calculationGetAmbientLightFactor();
double  calculationGetAmbientLightFactorOrg();

void   calculationSetKalmanK(double koef);
double calculationGetKalmanK();
void   calculationSetKalmanQ(double koef);
void   calculationSetKalmanThreshold(int indx, int koef);
void   calculationSetKalmanNumCheck(int num);
void   calculationEnableKalman(int enableKalman);
void   calculationSetKalmanKdiff(double koef);
void   calculationInitKalman();
void   calculationFalmanFiltering(uint16_t ** data);
double calculationSimpleKalmanFilter(double val, int indx, double koef);
double calculationKalmanFilter(int val, int indx, double q, double r);
double calculationVariance(double val, int n, int indx);

void   calculationCorrectFlimOffset(int enable);
void   calculationCorrectFlimGain(int enable);
void   calculationFlimCorrection(uint16_t **pData, int size);
int    calculationFlimMean(int numAveragingImages, int* data, int side);
void   calculationFlimGainOffset(int side);
int    calculationFlimCalibration(char* gates);
int    calculationCalibrateFlim(int mode);
int    calculateSaveFlimImage(int *data);
int    calculateLoadFlimImage(int *data);
int    calculateLoadFlimGainOffset(int side);
int    calculateSaveFlimGainOffset(int side);
int    calculateSetFlimCorrectionAvailable(int value);
int    calculateIsFlimCorrectionAvailable();

#endif




