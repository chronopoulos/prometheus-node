#ifndef LOGGER_H_
#define LOGGER_H_

#include <inttypes.h>


typedef enum {
	LOG_MODE_NONE=0,
	BINARY_GrayAndDCS=1,
} log_mode_t;

void logSetFile(char *filename);
void logMessage(char *message);
log_mode_t getLogMode(void);
void setLogMode(log_mode_t mode);
void logAddDCS(const uint16_t *data, const unsigned int dcs, const unsigned int width, const unsigned int height);
void logMoveCursor();
int  logEnableLogData(const unsigned int state);
int  logIsEnabled();
void logCSV();
void logTrigger();

#endif
