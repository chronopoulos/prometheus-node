#ifndef _HELPER_H_
#define _HELPER_H_

#include "evalkit_constants.h"
#include <sys/time.h>

int helperStringToHex(char *string);
int helperStringToInteger(char *string);
double helperStringToDouble(char *string);
int helperParseCommand(char buffer[], char stringArray[MAX_COMMAND_ARGUMENTS][MAX_COMMAND_ARGUMENT_LENGTH]);
int helperIntervalCheck(const struct timeval a, const struct timeval b, unsigned int ms);

#endif
