#ifndef CALIBRATION_H_
#define CALIBRATION_H_

#include <stdint.h>

#define DLL_LOCKS 4
#define PHASE_RANGE_HALF 1500
#define N_MEASUREMENTS_PHASE_SHIFT 10

#define TEMPERATURE_SLOPE  0.00395
#define TEMPERATURE_CALIBRATION_DELAY 270.0	// in d° (deziGrad)
//#define DRNU_CALIBRATION_BOX_LENGTH_CM  34.5


int16_t calibrationSearchBadPixelsWithSat();
int16_t calibrationSearchBadPixelsWithMin();
uint16_t calibrationGetBadPixels(int16_t* badPixels);
int  calibrationSetOffsetCM(const int value, const int deviceAddress);
void calibrationSetOffsetPhase(int value);
void calibrationSetDefaultOffsetPhase(int value);
int  calibrationGetOffsetPhase();
int  calibrationGetOffsetDefaultCM(const int deviceAddress);
int  calibrationEnableDefaultOffset(int state);
int  calibrationIsDefaultEnabled();
void calibrationSetNumberOfColumnsMax(const uint16_t value);
void calibrationSetNumberOfRowsMax(const uint16_t value);
int  calibrationGetOffsetDRNU();
int  calibrationDRNU();
int  calibrationDRNU_FromFile();
void calibrationCreateDRNU_LUT(int indx, int numAveragedFrames);
void configAddEndFileName(char* name, char* str);
int  calibrationSaveDRNU_LUT();
int  calibrationSaveDRNU_LUT_TestXY(char* fileName);
int  calibrationSaveDRNU_LUT_TestYX(char* fileName);
int  calibrationLoadDRNU_LUT();
void calibrationChangeDRNU_LUT();
void calibrationLinearRegresion(double* slope, double *offset);
double calibrationFindLine(int indexEnd);
void calibrationCorrectDRNU_LUT(double slope, double offset);
void calibrationCorrectDRNU_Error();
int  calibrationErrorCurveEndIndex();
void calibrationCreateDRNU_Images();
int  calibrationLoadDistanceImages(int index);
int  calibrationSaveDistanceImages();
int  calibrationSaveDistanceImagesTXT();
int  calibrationSaveTemperatureDRNU();
int  calibrationRunPreheat(int preheatTemp);
int16_t* calibrationLoadTemperatureDRNU();
int16_t* calibrateRenewDRNU(int stepMM);
int16_t* calibrateShowDRNU();
void calibrationUpdateFrequency();
int  calibrateGetoffsetPhaseDefaultFPN();
void calibrateSetDRNUCalibrateDelay(int time_uS);
void calibrateSetDRNUDiffTemp(int diffTemp);
void calibrateSetDRNUAverage(int numAveragedFrames);
int  calibrateGetDRNUAverage();
void calibrateSetpreHeatTemp(int tempPreheat);
int  calibrateGetpreHeatTemp();
int  calibrateGetDRNUDiffTemp();
int  calibrateGetDRNUCalibrateDelay();
int  calibrateGetTempCalib();
double   calibrateGetStepCalMM();
int      calibrateGetNumDistances();
void     calibrateSetNumDistances(int val);
void 	 calibrateSetHeight(int val);
int      calibrateGetHeight();
void 	 calibrateSetWidth(int val);
int      calibrateGetWidth();
int      calibrateGetDRNULutPixel(int indx, int y, int x);
uint16_t calibrateGetDistImagePixel(int indx);
void     calibrateSetTempCoef(double koef, int partType);
double   calibrateGetTempCoef(int partType);
double   calibrateGetSpeedOfLightDiv2();
void     calibrateSetSpeedOfLightDiv2(double val);
int calibrateGetCalibrationBoxLengthMM();
void calibrateSetCalibrationBoxLengthMM(int value);

//void saveCalibrateReg(int corrTableIdx);			//toDo Code in a calib regConfigFile
//void setCalibrateReg(int corrTableIdx);			//toDo Code in a calib regConfigFile


#endif
