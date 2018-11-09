#include "helper.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

/// \addtogroup helper
/// @{

/*!
 Converts a string to Hex
 @param string
 @returns value between 0x00 - 0xFF or -1 on error
 */
int helperStringToHex(char *string) {
	int i;
	sscanf(string, "%X", &i);
	if (i < 0x00 || i > 0xFF) {
		return -1;
	}
	return i;
}

/*!
 Converts a string to int
 @param string
 @returns int
 */
int helperStringToInteger(char *string) {
	int i;
	sscanf(string, "%i", &i);
	return i;
}


double helperStringToDouble(char *string) {
	double d;
	sscanf(string, "%lf", &d);
	return d;
}

/*!
 Parse command sent by client and split command
 @param buffer[] input buffer filled with request from client.
 @param stringArray for saving command and arguments
 @returns number of arguments.
 */
int helperParseCommand(char buffer[], char stringArray[MAX_COMMAND_ARGUMENTS][MAX_COMMAND_ARGUMENT_LENGTH]) {
	char delimiters[] = " ";
	char *token = strtok(buffer, delimiters);
	int count = 0;
	while (token != NULL && count < MAX_COMMAND_ARGUMENTS) {
		strncpy(stringArray[count++], token, MAX_COMMAND_ARGUMENT_LENGTH);
		token = strtok(NULL, delimiters);
	}
	return count - 1;
}

/*!
 Subtracts timevals a - b
 @param timeval a
 @param timeval b
 @param timeval ms interval
 returns 0 for interval not expired yet
*/
int helperIntervalCheck(const struct timeval a, const struct timeval b, unsigned int s) {
	if (s <= 0){
		return 0;
	}
	int seconds;
	seconds = (b.tv_sec - a.tv_sec);
	return seconds >= s;
}

/// @}
