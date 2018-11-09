#ifndef _BOOT_H_
#define _BOOT_H_

#include <stdint.h>

void bootInit();
int boot();
void bootRelease();
void bootUndo();
void bootUnexportAll();
void bootNormalMode(unsigned int deviceAddress);
unsigned int bootGetIcVersion(unsigned int deviceAddress);
unsigned int bootGetPartType(unsigned int deviceAddress);
unsigned int bootGetPartVersion(unsigned int deviceAddress);
uint16_t bootGetWaferID(unsigned int deviceAddress);
uint16_t bootGetChipID(unsigned int deviceAddress);
void bootReset();
void bootCloseEpc660_enable();

#endif
