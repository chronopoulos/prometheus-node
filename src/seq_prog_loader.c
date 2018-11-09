#include "seq_prog_loader.h"
#include "i2c.h"
#include "registers.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/// \addtogroup sequencer
/// @{

/*!
 Loads a sequencer program from a file
 @param filename
 @param deviceAddress i2c address of the chip (default 0x20)
 @returns On success, 0, on error -1
 */
int sequencerProgramLoad(char *filename, const int deviceAddress) {
	FILE *file;
	file = fopen(filename, "r");
	if (file == NULL){
		printf("couldn't open %s.\n", filename);
		return -1;
	}
	char line[256];
	fgets(line,sizeof(line), file);

	unsigned char *i2cData = malloc(8 * sizeof(unsigned char));
	i2cData[0] = 0x00;
	i2c(deviceAddress, 'w', SHUTTER_CONTROL, 1, &i2cData); //stop acquisition
	i2c(deviceAddress, 'w', SEQ_CONTROL, 1, &i2cData);     //disable sequencers
	i2cData[0] = 0x01;
	i2c(deviceAddress, 'w', SR_PROGRAM, 1, &i2cData);	   //enable pixel sequencer programming

	unsigned char memAddress = 0x00;
	i2cData[6] = 0x00;
	i2cData[7] = 0x0D;

	while (fgets(line, sizeof(line), file)) {
		char valueStr[9];
		valueStr[8] = '\0';
		strncpy(valueStr, line, 8);
		i2cData[5] = (unsigned char) strtol(valueStr, NULL, 2);

		strncpy(valueStr, line+8, 8);
		i2cData[4] = (unsigned char) strtol(valueStr, NULL, 2);

		strncpy(valueStr, line+16, 8);
		i2cData[3] = (unsigned char) strtol(valueStr, NULL, 2);

		strncpy(valueStr, line+24, 8);
		i2cData[2] = (unsigned char) strtol(valueStr, NULL, 2);

		strncpy(valueStr, line+32, 8);
		i2cData[1] = (unsigned char) strtol(valueStr, NULL, 2);

		i2cData[0] = memAddress;
		i2c(deviceAddress, 'w', SR_ADDRESS, 8, &i2cData);
		memAddress++;
	}
	fclose(file);

	i2cData[0] = 0x00;
	i2c(deviceAddress, 'w', SR_PROGRAM, 1, &i2cData);  //disable programming mode
	i2cData[0] = 0x03;
	i2c(deviceAddress, 'w', SEQ_CONTROL, 1, &i2cData); //enable sequencers
	return sequencerReadVersion(filename);
}

int sequencerReadVersion(char* fileName){
	char num[6] = {0,0,0,0,0,0};
	int i=0;
	while( fileName[i] != 'V' && fileName[i] != 'v' && fileName[i] != '\0') i++;
	i++;

	int j=0;
	while(fileName[i] != '.' && fileName[i] != '\0'){
		num[j] = fileName[i];
		i++;
		j++;
	}

	return atoi(num); //seqVersion
}

/// @}
