#define _GNU_SOURCE
#include "server.h"
#include "api.h"
#include "evalkit_constants.h"
#include "i2c.h"
#include "config.h"
#include "evalkit_illb.h"
#include "boot.h"
#include "pru.h"
#include "bbb_led.h"
#include "dll.h"
#include "read_out.h"
#include "log.h"
#include "version.h"
#include "helper.h"
#include "calibration.h"
#include "calculation.h"
#include "queue_imaging.h"

#include <stdio.h>
#include <strings.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>

/// \addtogroup server
/// @{


static pthread_t imageThread;

static int gStartGrayVideo    = 0;
static int gStartDCSVideo     = 0;
static int gStartDCSTOFandGrayVideo = 0;
static int gStartAmpDistVideo = 0;
static int gStartDistVideo    = 0;
static int gStartAmpVideo     = 0;
static int16_t gRunVideo      = 0;

enum threadType{
	StartGrayVideo,
	StartAmpVideo,
	StartDistVideo,
	StartAmpDistVideo,
	StartDCSVideo,
	StartDCSTOFandGrayVideo,
	AllVideo
};

typedef struct {
	int (* func)(uint16_t**);
}funPointer;

void imagingThread(int (* funcThread)(uint16_t**));
void handleRequest(int sock);
void signalHandler(int sig);
void serverSend( void* buffer, size_t size);
enum threadType pauseVideo(void);
void resumeVideo(enum threadType typeLast);
void serverStopThreads(enum threadType type);

int sockfd, newsockfd, pid;
socklen_t clilen;
struct sockaddr_in serv_addr, cli_addr;

static unsigned int deviceAddress;
unsigned char* pData;
int gSock = 0;

int16_t temps[6];
int16_t chipInfo[8];
static int dataSizeThread = 0;
static int stopThreadFlag = 1;

void imagingThread(int (* funcThread)(uint16_t**)){
	uint16_t* pMemm=NULL;
	stopThreadFlag = 0;

	while(gStartGrayVideo==1 || gStartAmpVideo==1 || gStartDistVideo==1 || gStartAmpDistVideo==1 || gStartDCSVideo==1 || gStartDCSTOFandGrayVideo==1){
		dataSizeThread = 2 * funcThread(&pMemm);
		enqueue(&pMemm, dataSizeThread);
	};

	stopThreadFlag = 1;
	printf("imagingThread: stopThreadFlag = %d\n", stopThreadFlag);	//TODO remove
}

/*!
 Starts TCP Server for communication with client application
 @param addressOfDevice i2c address of the chip (default 0x20)
 @return On error, -1 is returned.
 */
int startServer(const unsigned int addressOfDevice) {
	deviceAddress = addressOfDevice;
	signal(SIGQUIT, signalHandler); 	// add signal to signalHandler to close socket on crash or abort
	signal(SIGINT, signalHandler);
	signal(SIGPIPE, signalHandler);
	signal(SIGSEGV, signalHandler);
	pData = malloc(MAX_COMMAND_ARGUMENTS - 3 * sizeof(unsigned char));	//free???
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		return -1;
	}
	int optval = 1;
	// set SO_REUSEADDR on a socket to true
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval) < 0) {
		printf("Error setting socket option\n");
		return -1;
	}
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(TCP_PORT_NO);
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {	// bind server address to socket
		printf("Binding error: please try again in 20sec and make sure no other instance is running\n");
		return -1;
	}
	printf("Socket successfully opened\n");
	listen(sockfd, 5);						// tell socket that we want to listen for communication
	clilen = sizeof(cli_addr);

	// handle single connection
	printf("waiting for requests\n");
	bbbLEDEnable(1, 0);
	bbbLEDEnable(2, 0);
	bbbLEDEnable(3, 0);
	while (1) {
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		if (newsockfd < 0) {
			printf("couldn't accept\n");
			return -1;
		}
		handleRequest(newsockfd);
		close(newsockfd);
	}
	close(sockfd);
	return 0;
}

/* drop-in replacement for send() */
void send_lengthPrefixed(int sockfd, const void *buf, uint32_t dataSize, int flags) {

    send(sockfd, &dataSize, 4, flags);
    send(sockfd, buf, dataSize, flags);

}

/*!
 Handles Requests and writes answer into TCP socket
 @param sock TCP-socket to write response.
 @return On error, -1 is returned.
 */
void handleRequest(int sock) {
	bbbLEDEnable(3, 1);
	char buffer[TCP_BUFFER_SIZE];
	bzero(buffer, TCP_BUFFER_SIZE);
	int size = read(sock, buffer, TCP_BUFFER_SIZE);
	gSock = sock;
	if (buffer[size - 1] == '\n'){
		buffer[size - 1] = '\0';
	}
	char stringArray[MAX_COMMAND_ARGUMENTS][MAX_COMMAND_ARGUMENT_LENGTH];
	int argumentCount = helperParseCommand(buffer, stringArray);
	unsigned char response[TCP_BUFFER_SIZE];
	bzero(response, TCP_BUFFER_SIZE);
	int16_t answer;

	// show requests
	if( configPrint() > 0 ){
		int ir;
		for (ir = 0; ir < argumentCount + 1; ir++) {
			printf("%s ", stringArray[ir]);
		}
		printf("\n");
	}

	//COMMANDS
	if ((strcmp(stringArray[0], "readRegister") == 0 || strcmp(stringArray[0], "read") == 0 || strcmp(stringArray[0], "r") == 0) && argumentCount >= 1 && argumentCount < 3) {
		unsigned char *values;
		int v;
		int nBytes;
		if (argumentCount == 1) {
			nBytes = 1;
		} else {
			nBytes = helperStringToHex(stringArray[2]);
		}
		int registerAddress = helperStringToHex(stringArray[1]);
		if (nBytes > 0 && registerAddress >= 0) {
			values = malloc(nBytes * sizeof(unsigned char));
			int16_t responseValues[nBytes];
			apiReadRegister(registerAddress, nBytes, values, deviceAddress);
			for (v = 0; v < nBytes; v++) {
				responseValues[v] = values[v];
			}
			send_lengthPrefixed(sock, responseValues, nBytes * sizeof(int16_t), MSG_NOSIGNAL);
			free(values);
		} else {
			answer = -1;
			send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
		}
	} else if ((strcmp(stringArray[0], "writeRegister") == 0 || strcmp(stringArray[0], "write") == 0 || strcmp(stringArray[0], "w") == 0) && argumentCount > 1) {
		unsigned char *values = malloc((argumentCount - 1) * sizeof(unsigned char));
		int registerAddress = helperStringToHex(stringArray[1]);
		int i;
		int ok = 1;
		for (i = 0; i < argumentCount - 1; i++) {
			if (helperStringToHex(stringArray[i + 2]) >= 0) {
				values[i] = helperStringToHex(stringArray[i + 2]);
			} else {
				ok = 0;
			}
		}
		if (registerAddress > 0 && ok > 0) {
			answer = apiWriteRegister(registerAddress, argumentCount - 1, values, deviceAddress);
		} else {
			answer = -1;
		}
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
		free(values);
	}	else if (strcmp(stringArray[0], "enableImaging") == 0 && argumentCount == 1) {
		answer = apiSetEnableImaging(helperStringToInteger(stringArray[1]));
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "getBWSorted") == 0 && !argumentCount) {
		uint16_t *pMem = NULL;
		serverStopThreads(StartGrayVideo);

		if(gRunVideo == 1){
			if(gStartGrayVideo == 0 ){
				gStartGrayVideo = 1;
				queueInit();	// reset queue buffer
				pthread_create(&imageThread, NULL, (void*)imagingThread, apiGetBWSorted);
			}

			while(isQueueEmpty() == true)	usleep(10000);
			dequeue(&pMem);
			send_lengthPrefixed(sock, pMem, dataSizeThread, MSG_NOSIGNAL);

		}else{
			int dataSize = 2 * apiGetBWSorted(&pMem);
			send_lengthPrefixed(sock, pMem, dataSize, MSG_NOSIGNAL);
		}
	} else if (strcmp(stringArray[0], "getDCSSorted") == 0 && !argumentCount) {
		uint16_t *pMem = NULL;
		serverStopThreads(StartDCSVideo);

		if(gRunVideo == 1){
			if(gStartDCSVideo == 0 ){
				gStartDCSVideo = 1;
				queueInit();	// reset queue buffer
				pthread_create(&imageThread, NULL, (void*)imagingThread, apiGetDCSSorted);
			}

			while(isQueueEmpty() == true)	usleep(10000);
			dequeue(&pMem);
			send_lengthPrefixed(sock, pMem, dataSizeThread, MSG_NOSIGNAL);

		}else{
			int dataSize = 2 * apiGetDCSSorted(&pMem);
			send_lengthPrefixed(sock, pMem, dataSize, MSG_NOSIGNAL);
		}
	} else if (strcmp(stringArray[0], "getDCSTOFAndGrayscaleSorted") == 0 && !argumentCount) {
		uint16_t *pMem = NULL;
		serverStopThreads(StartDCSTOFandGrayVideo);

		if(gRunVideo == 1){
			if(gStartDCSTOFandGrayVideo == 0 ){
				gStartDCSTOFandGrayVideo = 1;
				queueInit();	// reset queue buffer
				pthread_create(&imageThread, NULL, (void*)imagingThread, apiGetDCSTOFeAndGrayscaleSorted);
			}

			while(isQueueEmpty() == true)	usleep(10000);
			dequeue(&pMem);
			send_lengthPrefixed(sock, pMem, dataSizeThread, MSG_NOSIGNAL);

		}else{
			int dataSize = 2 * apiGetDCSTOFeAndGrayscaleSorted(&pMem);
			send_lengthPrefixed(sock, pMem, dataSize, MSG_NOSIGNAL);
		}
	} else if (strcmp(stringArray[0], "getDistanceSorted") == 0 && !argumentCount) {
		uint16_t *pMem = NULL;
		serverStopThreads(StartDistVideo);

		if(gRunVideo == 1){
			if(gStartDistVideo == 0 ){
				gStartDistVideo = 1;
				queueInit();	// reset queue buffer
				pthread_create(&imageThread, NULL, (void*)imagingThread, apiGetDistanceSorted);
			}

			while(isQueueEmpty() == true)	usleep(10000);
			dequeue(&pMem);
			send_lengthPrefixed(sock, pMem, dataSizeThread, MSG_NOSIGNAL);

		}else{
			int dataSize = 2 * apiGetDistanceSorted(&pMem);
			send_lengthPrefixed(sock, pMem, dataSize, MSG_NOSIGNAL);
		}

	} else if (strcmp(stringArray[0], "getAmplitudeSorted") == 0 && !argumentCount) {
		uint16_t *pMem = NULL;
		serverStopThreads(StartAmpVideo);

		if(gRunVideo == 1){
			if(gStartAmpVideo == 0 ){
				gStartAmpVideo = 1;
				queueInit();	// reset queue buffer
				pthread_create(&imageThread, NULL, (void*)imagingThread, apiGetAmplitudeSorted);
			}

			while(isQueueEmpty() == true)	usleep(10000);
			dequeue(&pMem);
			send_lengthPrefixed(sock, pMem, dataSizeThread, MSG_NOSIGNAL);

		}else{
			int dataSize = 2 * apiGetAmplitudeSorted(&pMem);
			send_lengthPrefixed(sock, pMem, dataSize, MSG_NOSIGNAL);
		}

	} else if (strcmp(stringArray[0], "getDistanceAndAmplitudeSorted") == 0 && !argumentCount) {
		uint16_t* pMem=NULL;
		serverStopThreads(StartAmpDistVideo);

		if(gRunVideo == 1){
			if(gStartAmpDistVideo == 0 ){
				gStartAmpDistVideo = 1;
				queueInit();	// reset queue buffer
				pthread_create(&imageThread, NULL, (void*)imagingThread, apiGetDistanceAndAmplitudeSorted);
			}

			while(isQueueEmpty() == true)	usleep(10000);
			dequeue(&pMem);
			send_lengthPrefixed(sock, pMem, dataSizeThread, MSG_NOSIGNAL);

		}else{
			int dataSize = 2 * apiGetDistanceAndAmplitudeSorted(&pMem);
			send_lengthPrefixed(sock, pMem, dataSize, MSG_NOSIGNAL);
		}

	}else if (strcmp(stringArray[0], "startVideo") == 0 && !argumentCount) {
		gRunVideo = 1;
		send_lengthPrefixed(sock, &gRunVideo, sizeof(int16_t), MSG_NOSIGNAL);

	}else if (strcmp(stringArray[0], "stopVideo") == 0 && !argumentCount) {
		serverStopThreads(AllVideo);
		gRunVideo = 0;
		send_lengthPrefixed(sock, &gRunVideo, sizeof(int16_t), MSG_NOSIGNAL);
	}else if (strcmp(stringArray[0], "enableIllumination") == 0 && argumentCount == 1) {
		if (!helperStringToInteger(stringArray[1])){
			illuminationDisable();
			answer = 0;
		} else {
			illuminationEnable();
			answer = 1;
		}
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "setABS") == 0 && argumentCount == 1) {
		answer = apiEnableABS(helperStringToInteger(stringArray[1]), deviceAddress);
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "enableVerticalBinning") == 0 && argumentCount == 1) {
		answer = apiEnableVerticalBinning(helperStringToInteger(stringArray[1]), deviceAddress);
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "enableHorizontalBinning") == 0 && argumentCount == 1) {
		answer = apiEnableHorizontalBinning(helperStringToInteger(stringArray[1]), deviceAddress);
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "setRowReduction") == 0 && argumentCount == 1) {
		answer = apiSetRowReduction(helperStringToInteger(stringArray[1]), deviceAddress);
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "loadConfig") == 0 && argumentCount == 1) {
		serverStopThreads(AllVideo);
		answer = apiLoadConfig(helperStringToInteger(stringArray[1]), deviceAddress);
		if (answer == 1){
			illuminationEnable();
			printf("illumination enabled\n");
		}else{
			illuminationDisable();
			printf("illumination disable\n");
		}
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	}else if (strcmp(stringArray[0], "setIntegrationTime2D") == 0 && argumentCount == 1) {
		answer = apiSetIntegrationTime2D(helperStringToInteger(stringArray[1]), deviceAddress);
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "setIntegrationTime3D") == 0 && argumentCount == 1) {
		answer = apiSetIntegrationTime3D(helperStringToInteger(stringArray[1]), deviceAddress);
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "setIntegrationTime3DHDR") == 0 && argumentCount == 1) {
		answer = apiSetIntegrationTime3DHDR(helperStringToInteger(stringArray[1]), deviceAddress);
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "getImagingTime") == 0 && argumentCount == 0) {
		answer = apiGetImagingTime();
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "filterMode") == 0 && argumentCount == 1) {
		answer=apiEnableSquareAddDcs(helperStringToInteger(stringArray[1]));
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "nloopFilter") == 0 && argumentCount == 1) {
		answer=apiSetNfilterLoop(helperStringToInteger(stringArray[1]));
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "enableArgCheck") == 0 && argumentCount == 1) {
		answer=apiEnableAddArgThreshold(helperStringToInteger(stringArray[1]));
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "setAddArgThreshold") == 0 && argumentCount == 1) {
		answer = apiSetAddArgThreshold(helperStringToInteger(stringArray[1]));
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "setAddArgMin") == 0 && argumentCount == 1) {
		answer = apiSetAddArgMin(helperStringToInteger(stringArray[1]));
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "setAddArgMax") == 0 && argumentCount == 1) {
		answer = apiSetAddArgMax(helperStringToInteger(stringArray[1]));
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "setMinAmplitude") == 0 && argumentCount == 1) {
		answer = apiSetMinAmplitude(helperStringToInteger(stringArray[1]));
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "getMinAmplitude") == 0 && argumentCount == 0) {
		answer = apiGetMinAmplitude();
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "enableDualMGX") == 0 && argumentCount == 1) {
		answer = apiEnableDualMGX(helperStringToInteger(stringArray[1]), deviceAddress);
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "enableHDR") == 0 && argumentCount == 1) {
		answer = apiEnableHDR(helperStringToInteger(stringArray[1]), deviceAddress);
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "enablePiDelay") == 0 && argumentCount == 1) {
		answer = apiEnablePiDelay(helperStringToInteger(stringArray[1]), deviceAddress);
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "setModulationFrequency") == 0 && argumentCount == 1) {
		int16_t calibrationType = 0;
		enum threadType typeLastPaused=AllVideo;
		if(gRunVideo)
			typeLastPaused=pauseVideo();
		answer = apiSetModulationFrequency(helperStringToInteger(stringArray[1]), deviceAddress);
		calibrationType=apiGetModulationFrequencyCalibration(helperStringToInteger(stringArray[1]));
		if(calibrationType<0){
			apiCorrectDRNU(0);
		}
		if(gRunVideo)
			resumeVideo(typeLastPaused);

		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "getModulationFrequencies") == 0 && argumentCount == 0) {
		int *answerp = apiGetModulationFrequencies();
		send_lengthPrefixed(sock, answerp, configGetModFreqCount() * sizeof(int), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "getCalibrationTypeForFreqIdx") == 0 && argumentCount == 1) {
		answer = apiGetModulationFrequencyCalibration(helperStringToInteger(stringArray[1]));
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "setROI") == 0 && argumentCount == 4) {
		answer = apiSetROI(helperStringToInteger(stringArray[1]), helperStringToInteger(stringArray[2]), helperStringToInteger(stringArray[3]), helperStringToInteger(stringArray[4]), deviceAddress);
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "setOffset") == 0 && argumentCount == 1) {
		answer = apiSetOffset(helperStringToInteger(stringArray[1]), deviceAddress);
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "getOffset") == 0) {
		answer = apiGetOffset(deviceAddress);
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "enableDefaultOffset") == 0 && argumentCount == 1) {
		enum threadType typeLastPaused=AllVideo;
		if(gRunVideo)
			typeLastPaused=pauseVideo();
		answer = apiEnableDefaultOffset(helperStringToInteger(stringArray[1]));
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
		if(gRunVideo)
			resumeVideo(typeLastPaused);
	}else if (strcmp(stringArray[0], "getBadPixels") == 0) {
		int16_t *badPixels = malloc(100000 * sizeof(int16_t));
		answer = apiGetBadPixels(badPixels);
		if (answer < 0) send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
		else			send_lengthPrefixed(sock, badPixels, answer * sizeof(int16_t), MSG_NOSIGNAL);
		free(badPixels);
	} else if (strcmp(stringArray[0], "getTemperature") == 0) {
		int16_t size = apiGetTemperature(deviceAddress, temps);
		send_lengthPrefixed(sock, &temps, size * sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "getAveragedTemperature") == 0) {
		int16_t answer = apiGetAveragedTemperature(deviceAddress);
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "getChipInfo") == 0) {
		int16_t size = apiGetChipInfo(deviceAddress, chipInfo);
		send_lengthPrefixed(sock, &chipInfo, size * sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "setHysteresis") == 0 && argumentCount == 1) {
		answer = apiSetHysteresis(helperStringToInteger(stringArray[1]));
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "selectMode") == 0 && argumentCount == 1) {
		answer = apiSelectMode(helperStringToInteger(stringArray[1]), deviceAddress);
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "selectPolynomial") == 0 && argumentCount == 2) {
		answer = apiSelectPolynomial(helperStringToInteger(stringArray[1]), helperStringToInteger(stringArray[2]), deviceAddress);
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "enableImageCorrection") == 0 && argumentCount == 1) {
		answer = apiEnableImageCorrection(helperStringToInteger(stringArray[1]));
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "setImageAveraging") == 0 && argumentCount == 1) {
		answer = apiSetImageAveraging(helperStringToInteger(stringArray[1]));
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "setImageProcessing") == 0 && argumentCount == 1) {
		answer = apiSetImageProcessing(helperStringToInteger(stringArray[1]));
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "setImageDifferenceThreshold") == 0 && argumentCount == 1) {
		answer = apiSetImageDifferenceThreshold(helperStringToInteger(stringArray[1]));
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	}else if (strcmp(stringArray[0], "getIcVersion") == 0 && argumentCount == 0) {
		answer = apiGetIcVersion();
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	}else if (strcmp(stringArray[0], "getPartVersion") == 0 && argumentCount == 0) {
		answer = apiGetPartVersion();
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	}else if (strcmp(stringArray[0], "enableSaturation") == 0 && argumentCount == 1) {
		answer = apiEnableSaturation(helperStringToInteger(stringArray[1]));
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	}else if (strcmp(stringArray[0], "enableAdcOverflow") == 0 && argumentCount == 1) {
		answer = apiEnableAdcOverflow(helperStringToInteger(stringArray[1]));
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	}else if (strcmp(stringArray[0], "isFLIM") == 0 && !argumentCount) {
		answer = apiIsFLIM();
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "FLIMSetT1") == 0 && argumentCount == 1) {
		answer = apiFLIMSetT1(helperStringToInteger(stringArray[1]));
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "FLIMSetT2") == 0 && argumentCount == 1) {
		answer = apiFLIMSetT2(helperStringToInteger(stringArray[1]));
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "FLIMSetT3") == 0 && argumentCount == 1) {
		answer = apiFLIMSetT3(helperStringToInteger(stringArray[1]));
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "FLIMSetT4") == 0 && argumentCount == 1) {
		answer = apiFLIMSetT4(helperStringToInteger(stringArray[1]));
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "FLIMSetTREP") == 0 && argumentCount == 1) {
		answer = apiFLIMSetTREP(helperStringToInteger(stringArray[1]));
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "FLIMSetRepetitions") == 0 && argumentCount == 1) {
		answer = apiFLIMSetRepetitions(helperStringToInteger(stringArray[1]));
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "FLIMSetFlashDelay") == 0 && argumentCount == 1) {
		answer = apiFLIMSetFlashDelay(helperStringToInteger(stringArray[1]));
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "FLIMSetFlashWidth") == 0 && argumentCount == 1) {
		answer = apiFLIMSetFlashWidth(helperStringToInteger(stringArray[1]));
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "FLIMGetStep") == 0 && argumentCount == 0) {
		answer = apiFLIMGetStep();
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "searchBadPixels") == 0 && argumentCount == 0) {
		if (calibrationSearchBadPixelsWithMin() < 0) answer = -1;
		else answer = 0;
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	}else if (strcmp(stringArray[0], "dumpAllRegisters") == 0 && argumentCount == 0) {
		int16_t dumpData[256];
		dumpAllRegisters(deviceAddress, dumpData);
		send_lengthPrefixed(sock, dumpData, sizeof(int16_t) * 256, MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "isFlimCorrectionAvailable") == 0 && argumentCount == 0) {
		answer = apiIsFlimCorrectionAvailable();
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "calibrateFlim") == 0  && argumentCount == 1) {
		answer = apiCalibrateFlim(stringArray[1]);
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "correctFlimOffset") == 0 && argumentCount == 1) {
		answer = apiCorrectFlimOffset(helperStringToInteger(stringArray[1]));
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "correctFlimGain") == 0 && argumentCount == 1) {
		answer = apiCorrectFlimGain(helperStringToInteger(stringArray[1]));
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "calibrateGrayscale") == 0 && argumentCount == 1) {
		answer = apiCalibrateGrayscale(helperStringToInteger(stringArray[1]));
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "correctGrayscaleOffset") == 0 && argumentCount == 1) {
		answer = apiCorrectGrayscaleOffset(helperStringToInteger(stringArray[1]));
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "correctGrayscaleGain") == 0 && argumentCount == 1) {
		answer = apiCorrectGrayscaleGain(helperStringToInteger(stringArray[1]));
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "calibrateDRNU") == 0 && argumentCount == 0) {
		answer = apiCalibrateDRNU(helperStringToInteger(stringArray[1]));
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "correctDRNU") == 0 && argumentCount == 1) {
		answer = apiCorrectDRNU(helperStringToInteger(stringArray[1]));
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "setDRNUDelay") == 0 && argumentCount == 1) {
			answer = apiSetDRNUDelay(helperStringToInteger(stringArray[1]));
			send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "setDRNUDiffTemp") == 0 && argumentCount == 1) {
			answer = apiSetDRNUDiffTemp(helperStringToDouble(stringArray[1]));
			send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "setDRNUAverage") == 0 && argumentCount == 1) {
			answer = apiSetDRNUAverage(helperStringToInteger(stringArray[1]));
			send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "setPreheatTemp") == 0 && argumentCount == 1) {
		answer = apiSetpreHeatTemp(helperStringToInteger(stringArray[1]));
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "correctTemperature") == 0 && argumentCount == 1) {
		answer = apiCorrectTemperature(helperStringToInteger(stringArray[1]));
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "correctAmbientLight") == 0 && argumentCount == 1) {
			answer = apiCorrectAmbientLight(helperStringToInteger(stringArray[1]));
			send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "renewDRNU") == 0 && argumentCount == 1) {
			int16_t* data = apiRenewDRNU(helperStringToInteger(stringArray[1]));
			send_lengthPrefixed(sock, data, 100 * sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "showDRNU") == 0 && argumentCount == 0) {
			int16_t* data = apiShowDRNU();
			send_lengthPrefixed(sock, data, 100 * sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "loadTemperatureDRNU") == 0 && argumentCount == 0) {
			int16_t* data = apiLoadTemperatureDRNU();
			send_lengthPrefixed(sock, data, 50 * sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "isGrayscaleCorrectionAvailable") == 0 && argumentCount == 0) {
		answer = apiIsEnabledGrayscaleCorrection();
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);\
	} else if (strcmp(stringArray[0], "isDRNUAvailable") == 0 && argumentCount == 0) {
		answer = apiIsEnabledDRNUCorrection();
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "isTemperatureCorrectionEnabled") == 0 && argumentCount == 0) {
		answer = apiIsEnabledTemperatureCorrection();
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "setKalmanTempK") == 0 && argumentCount == 1) {
		double value = apiSetChipTempSimpleKalmanK(helperStringToDouble(stringArray[1]));
		send_lengthPrefixed(sock, &value, sizeof(double), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "getKalmanTempK") == 0 && argumentCount == 0) {
		double value = apiGetChipTempSimpleKalmanK();
		send_lengthPrefixed(sock, &value, sizeof(double), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "isAmbientLightCorrectionEnabled") == 0 && argumentCount == 0) {
		answer = apiIsEnabledAmbientLightCorrection();
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "setAmbientLightFactor") == 0 && argumentCount == 1) {
			answer = apiSetAmbientLightFactor(helperStringToInteger(stringArray[1]));
			send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "print") == 0 && argumentCount == 1) {
		answer = apiPrint(helperStringToInteger(stringArray[1]));
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "Kalman") == 0 && argumentCount == 1) {
		answer = apiEnableKalman(helperStringToInteger(stringArray[1]));
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "setKalmanKdiff") == 0 && argumentCount == 1) {
		answer = apiSetKalmanKdiff(helperStringToDouble(stringArray[1]));
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "setKalmanK") == 0 && argumentCount == 1) {
		answer = apiSetKalmanK(helperStringToDouble(stringArray[1]));
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "setKalmanQ") == 0 && argumentCount == 1) {
		answer = apiSetKalmanQ(helperStringToDouble(stringArray[1]));
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "setKalmanThreshold") == 0 && argumentCount == 1) {
		answer = apiSetKalmanThreshold(helperStringToInteger(stringArray[1]));
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "setKalmanNumCheck") == 0 && argumentCount == 1) {
		answer = apiSetKalmanNumCheck(helperStringToInteger(stringArray[1]));
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "setKalmanThreshold2") == 0 && argumentCount == 1) {
		answer = apiSetKalmanThreshold2(helperStringToInteger(stringArray[1]));
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "setTempCoef") == 0 && argumentCount == 1) {
		answer = apiSetTempCoef(helperStringToDouble(stringArray[1]));
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "setSpeedOfLight") == 0 && argumentCount == 1) {
		answer = apiSetSpeedOfLight(helperStringToDouble(stringArray[1]));
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "getSpeedOfLight") == 0 && argumentCount == 0) {
		double value = apiGetSpeedOfLightDev2();
		send_lengthPrefixed(sock, &value, sizeof(double), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "tof") == 0 && argumentCount == 0) {
		answer = apiTOF();
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "flim") == 0 && argumentCount == 0) {
		answer = apiFLIM();
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "extClkGen") == 0 && argumentCount == 1) {
		answer = apiSetExtClkGen(helperStringToInteger(stringArray[1]));
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "test") == 0 && argumentCount == 1) {
		answer = apiTest(helperStringToInteger(stringArray[1]));
		send_lengthPrefixed(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "version") == 0 && !argumentCount) {
		int16_t version = apiGetVersion();
		send_lengthPrefixed(sock, &version, sizeof(int16_t), MSG_NOSIGNAL);
	}
	// unknown command
	else {
		printf("unknown command -> %s\n", stringArray[0]);
		int16_t response = -1;
		send_lengthPrefixed(sock, &response, sizeof(int16_t), MSG_NOSIGNAL);
	}
	bbbLEDEnable(3, 0);
}

/*!
 Handles signals
 @param sig signal ID
 */
void signalHandler(int sig) {
	printf("caught signal %i .... going to close application\n", sig);
	close(sockfd);
	close(newsockfd);
	bbbLEDEnable(1, 1);
	bbbLEDEnable(2, 1);
	bbbLEDEnable(3, 1);
	exit(0);
}

void serverSend( void* buffer, size_t size){
	send(gSock, buffer, size, MSG_NOSIGNAL);
}

enum threadType pauseVideo(void){

	enum threadType typeLast;

	if(gStartGrayVideo)
		typeLast = StartGrayVideo;
	if(gStartAmpVideo)
		typeLast = StartAmpVideo;
	if(gStartDistVideo)
		typeLast = StartDistVideo;
	if(gStartAmpDistVideo)
		typeLast = StartAmpDistVideo;
	if(gStartDCSVideo)
		typeLast = StartDCSVideo;
	if(gStartDCSTOFandGrayVideo)
		typeLast = StartDCSTOFandGrayVideo;

	serverStopThreads(AllVideo); //reset+stop all threads egg. gStartGrayVideo
	//gRunVideo = 0; 				//no thread start possible without "startVideo"
	return typeLast;
}

void resumeVideo(enum threadType typeLast){

	switch(typeLast){
		case StartGrayVideo:
			//gRunVideo = 1;
			gStartGrayVideo=1;
			queueInit();
			pthread_create(&imageThread, NULL, (void*)imagingThread, apiGetBWSorted);
			printf("resumedVideo StartGrayVideo\n");
			break;
		case StartAmpVideo:
			//gRunVideo = 1;
			gStartAmpVideo=1;
			queueInit();
			pthread_create(&imageThread, NULL, (void*)imagingThread, apiGetAmplitudeSorted);
			printf("resumedVideo StartAmpVideo\n");
			break;
		case StartDistVideo:
			//gRunVideo = 1;
			gStartDistVideo=1;
			queueInit();
			pthread_create(&imageThread, NULL, (void*)imagingThread, apiGetDistanceSorted);
			printf("resumedVideo StartDistVideo\n");
			break;
		case StartAmpDistVideo:
			//gRunVideo = 1;
			gStartAmpDistVideo=1;
			queueInit();
			pthread_create(&imageThread, NULL, (void*)imagingThread, apiGetDistanceAndAmplitudeSorted);
			printf("resumedVideo StartAmpDistVideo\n");
			break;
		case StartDCSVideo:
			//gRunVideo = 1;
			gStartDCSVideo=1;
			queueInit();
			pthread_create(&imageThread, NULL, (void*)imagingThread, apiGetDCSSorted);
			printf("resumedVideo StartGrayVideo\n");
			break;
		case StartDCSTOFandGrayVideo:
			//gRunVideo = 1;
			gStartDCSTOFandGrayVideo=1;
			queueInit();
			pthread_create(&imageThread, NULL, (void*)imagingThread, apiGetDCSTOFeAndGrayscaleSorted);
			printf("resumedVideo StartDCSTOFandGrayVideo\n");
			break;
		default:
			//no video mode
			break;
	}
}

void serverStopThreads(enum threadType type){
	switch(type){

	case StartGrayVideo:
		if(gStartAmpVideo==1 || gStartAmpDistVideo==1 || gStartDistVideo==1 || gStartDCSVideo==1 || gStartDCSTOFandGrayVideo==1){
			gStartAmpVideo     = 0;
			gStartAmpDistVideo = 0;
			gStartDCSVideo     = 0;
			gStartDCSTOFandGrayVideo=0;
			gStartDistVideo    = 0;
			gStartDCSTOFandGrayVideo = 0;
			while(stopThreadFlag != 1) usleep(50000);
		}
		break;

	case StartAmpVideo:
		if(gStartGrayVideo==1 || gStartAmpDistVideo==1 || gStartDistVideo==1 || gStartDCSVideo==1 || gStartDCSTOFandGrayVideo==1){
			gStartGrayVideo    = 0;
			gStartAmpDistVideo = 0;
			gStartDCSVideo     = 0;
			gStartDCSTOFandGrayVideo=0;
			gStartDistVideo    = 0;
			while(stopThreadFlag != 1) usleep(50000);
		}
		break;

	case StartDistVideo:
		if(gStartGrayVideo==1 || gStartAmpVideo==1 || gStartAmpDistVideo==1 || gStartDCSVideo==1 || gStartDCSTOFandGrayVideo==1){
			gStartGrayVideo    = 0;
			gStartAmpVideo     = 0;
			gStartAmpDistVideo = 0;
			gStartDCSVideo     = 0;
			gStartDCSTOFandGrayVideo=0;
			while(stopThreadFlag != 1) usleep(50000);
		}
		break;

	case StartAmpDistVideo:
		if(gStartGrayVideo==1 || gStartAmpVideo==1 || gStartDistVideo==1 || gStartDCSVideo==1 || gStartDCSTOFandGrayVideo==1){
			gStartGrayVideo    = 0;
			gStartAmpVideo     = 0;
			gStartDCSVideo     = 0;
			gStartDCSTOFandGrayVideo=0;
			gStartDistVideo    = 0;
			while(stopThreadFlag != 1) usleep(50000);
		}
		break;

	case StartDCSVideo:
		if(gStartGrayVideo==1 || gStartAmpVideo==1 || gStartAmpDistVideo==1 || gStartDistVideo==1 || gStartDCSTOFandGrayVideo==1){
			gStartGrayVideo    = 0;
			gStartAmpVideo     = 0;
			gStartAmpDistVideo = 0;
			gStartDistVideo    = 0;
			gStartDCSTOFandGrayVideo=0;
			while(stopThreadFlag != 1) usleep(50000);
		}
		break;

	case StartDCSTOFandGrayVideo:
		if(gStartGrayVideo==1 || gStartAmpVideo==1 || gStartAmpDistVideo==1 || gStartDistVideo==1 || gStartDCSVideo==1){
			gStartGrayVideo    = 0;
			gStartAmpVideo     = 0;
			gStartDCSVideo     = 0;
			gStartAmpDistVideo = 0;
			gStartDistVideo    = 0;
			while(stopThreadFlag != 1) usleep(50000);
		}
		break;

	default:
		if(gStartGrayVideo==1 || gStartAmpVideo==1 || gStartAmpDistVideo==1 || gStartDistVideo==1 || gStartDCSVideo==1 || gStartDCSTOFandGrayVideo==1){
			gStartGrayVideo    = 0;
			gStartAmpVideo     = 0;
			gStartAmpDistVideo = 0;
			gStartDCSVideo     = 0;
			gStartDCSTOFandGrayVideo=0;
			gStartDistVideo    = 0;
			while(stopThreadFlag != 1) usleep(50000);
		}
	}
}

/// @}
