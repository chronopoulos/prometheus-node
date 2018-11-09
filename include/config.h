#ifndef _CONFIG_H_
#define _CONFIG_H_

#define EPC660_MAX_INTEGRATION_TIME 100000
#define EPC660_MAX_COL 327
#define EPC660_MAX_ROW 125

#define MAX_INTEGRATION_TIME_2D 1000000
#define MAX_INTEGRATION_TIME_3D 4000

#define CORR_BG_AVR_MODE   0
#define CORR_BG_LINE_MODE  2
#define CORR_BG_PIXEL_MODE 3


#include <stdint.h>
#include <stdbool.h>

enum partType {
	EPC502_OBS = 1,
	EPC660     = 2,
	EPC502     = 3,
	EPC635     = 4,
	EPC503     = 5
};

typedef enum {
	GRAYSCALE, THREED, FLIM, NumberOfModes
} op_mode_t;
int getModeCount();

typedef enum {
	MOD_FREQ_20000KHZ=0,
	MOD_FREQ_10000KHZ=1,
	MOD_FREQ_5000KHZ=2,
	MOD_FREQ_2500KHZ=3,
	MOD_FREQ_1250KHZ=4,
	MOD_FREQ_625KHZ=5,
	MOD_FREQ_20000KHZFace=6,
	MOD_FREQ_36000KHZ=7,
	NumberOfTypes = 7
} mod_freq_t;

typedef enum {
	NONE = 0,
	GRAYSCALE_DRNU_DSNU=1,
	DRNU_10_12MHZ_wABS_wPdelay_coarse=2,
	DRNU_20_24MHZ_wABS_wPdelay_coarse=3,
	DRNU_10_12MHZ_wABS_wPdelay_fine=4,
	DRNU_20_24MHZ_wABS_wPdelay_fine=5,
	DRNU_30_36MHZ_wABS_wPdelay_fine=6,
	DRNU_20_24MHZ_woABS_wPdelay_fine=0x15,	//= DRNU_20_24MHZ_wABS_wPdelay_fine & 0x10
} calibration_t;

typedef enum {
		EPC660_006=26,EPC660_007=27,			//epc660 devices = ("NumberOfPartType"= 2,+version)
		EPC502_006=36,EPC502_007=37,			//epc502 devices = ("NumberOfPartType"= 3,+version)
		EPC635_000=40,EPC635_001=41,			//epc635 devices = ("NumberOfPartType"= 4,+version)
		EPC503_000=50,EPC503_001=51,			//epc503 devices = ("NumberOfPartType"= 5,+version)
}chip_type_t;

int configGetModFreqCount();
int configSetModFreqCount(int numberFrequencies);
int configInit(int deviceAddress);
int configLoad(const op_mode_t mode, const int deviceAddress);
op_mode_t configGetCurrentMode();
int configSetIntegrationTime(const int us, const int deviceAddress, const int valueAddress, const int loopAddress);
int configSetIntegrationTime2D(const int us, const int deviceAddress);
int configGetIntegrationTime2D();
int configGetIntegrationTime3D();
int configGetIntegrationTime3DHDR();
int16_t configGetImagingTime();
int configEnableSquareAddDcs(const int squareAddSquareDcs);
int configStatusEnableSquareAddDcs();
int16_t setNfilterLoop(int16_t nLoop);
int16_t getNfilterLoop();
int configEnableAddArg(const int enableAddArg);
int configStatusEnableAddArg();
int configSetDcsAdd_threshold(const int thresholdAddArg);
int configGetDcsAdd_threshold();
int configSetAddArgMin(const int argMin);
int configSetAddArgMax(const int argMax);
int configSetIntegrationTime3D(const int us, const int deviceAddress);
int configSetIntegrationTime3DHDR(const int us, const int deviceAddress);
void configSetIntegrationTimesHDR();
int configSetModulationFrequency(const mod_freq_t freq, const int deviceAddress);
void configFindModulationFrequencies();
void configEnableDualMGX(const int state, const int deviceAddress);
void configSetDCS(unsigned char nDCS, const int deviceAddress);
uint16_t configGetModulationFrequency(const int deviceAddress);
int* configGetModulationFrequencies();
void setModulationFrequencyCalibration(calibration_t calibrationMode,mod_freq_t freq);
calibration_t getModulationFrequencyCalibration(mod_freq_t freq);
mod_freq_t configGetModFreqMode();
void configEnableABS(const unsigned int state, const int deviceAddress);
int configIsABS(const int deviceAddress);
uint16_t getConfigBitMask();
void setConfigBitMask(uint16_t mask);
void configSetMode(const int select, const int deviceAddress);
void configSetDeviceAddress(const int deviceAddress);
int configGetDeviceAddress();
int configIsFLIM();
int configGetSysClk();
double configGetTModClk();
double configGetTPLLClk();
void configSetIcVersion(unsigned int value);
unsigned int configGetIcVersion();
void configSetPartType(unsigned int value);
void configSetPartVersion(unsigned int value);
unsigned int configGetPartVersion();
unsigned int configGetPartType();
void configSetWaferID(const uint16_t ID);
uint16_t configGetWaferID();
void configSetChipID(const uint16_t ID);
uint16_t configGetChipID();
uint16_t configGetSequencerVersion();

unsigned char configGetGX();
unsigned char configGetGY();
unsigned char configGetGM();
unsigned char configGetGL();
unsigned char configGetGC();
int			  configGetGZ();

int  configIsEnabledGrayscaleCorrection();
int  configIsEnabledDRNUCorrection();
int  configIsEnabledTemperatureCorrection();
double setChipTempSimpleKalmanK(double K);
double getChipTempSimpleKalmanK();
int  configIsEnabledAmbientLightCorrection();
int	 setCalibNearRangeStatus(int status);
int	 getCalibNearRangeStatus();
int  configSetPrint(int val);
int  configPrint();
int  configLoadParameters();
int  configSaveParameters();
int  configRestartTOF();
int  configRestartFLIM();
void configInitRegisters(op_mode_t mode);
int  print(bool val, int a, char* format, ...);

#endif
