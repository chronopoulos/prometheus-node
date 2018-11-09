#ifndef _DLL_H_
#define _DLL_H_

#define N_LOCKS_REQ 10
#define N_CONT_LOCKS 5
#define WEIGHT_PREV 0.75

int dllBypassDLL(const int state, const int deviceAddress);
int dllEnablePreheat(const int state, const int deviceAddress);
int dllLockAutomatically(const int deviceAddress);
int dllInitialize(const int deviceAddress);
int dllDetermineAndSetDemodulationDelay(const int deviceAddress);
int dllGetCoarseCtrlExt();
int dllGetFineCtrlExt();
void dllResetDemodulationDelay(const int deviceAddress);

#endif
