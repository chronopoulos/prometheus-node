#include "boot.h"
#include "i2c.h"
#include "config.h"
#include "server.h"
#include "bbb_led.h"
#include "version.h"
#include "evalkit_illb.h"
#include "is5351A_clkGEN.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
	printf("Version %i.%i.%i\n", getMajor(), getMinor(), getPatch());
	bbbLEDEnable(3, 1);
	int deviceAddress = boot();
	if (deviceAddress < 0){
		bbbLEDEnable(1, 1);
		bbbLEDEnable(2, 0);
		bbbLEDEnable(3, 1);
		printf("Could not boot chip.\n");
		exit(-1);
	}
	printf("init\n");
	illuminationDisable();
	configSetDeviceAddress(deviceAddress);
	configInit(deviceAddress);
	illuminationEnable();

	printf("Starting server...\n");
	startServer(deviceAddress);
	return 0;
}
