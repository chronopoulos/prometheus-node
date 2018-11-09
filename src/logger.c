#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

/// \addtogroup output
/// @{

static char logfile[128];
static log_mode_t logMode=LOG_MODE_NONE;

void logSetFile(char *filename){
	mkdir("logs", 0777);
	strcpy(logfile,filename);
}

void logMessage(char *message){
	FILE *fp;
	char path[256] = "logs/";
	strcat(path, logfile);
	fp = fopen(path, "a+");
	time_t now = time(0);
	char time[100];
	strftime (time,100,"%Y-%m-%d %H:%M:%S", localtime (&now));
	fprintf(fp, "%s %s\n", time, message);
	fclose(fp);
}

log_mode_t getLogMode(void){
	return logMode;
}

void setLogMode(log_mode_t mode){
	logMode = mode;
}


/// @}
