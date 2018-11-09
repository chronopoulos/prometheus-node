#ifndef _HYSTERESIS_H_
#define _HYSTERESIS_H_

int hysteresisUpdate(const int position, const int value);
int hysteresisSetThresholds(const int minAmplitude, const int value);
void hysteresisUpdateThresholds(const int minAmplitude);
int hysteresisIsEnabled(const int position);

#endif
