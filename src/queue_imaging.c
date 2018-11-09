/**
 *
 * @file	FWO_queue.c
 *
 *
 * @author	S. Honegger
 *
 * @date	20.05.2015
 *
 * @version	1.2
 */

//============================================================================
//includes

#include "queue_imaging.h"
#include "evalkit_constants.h"

#include <stdbool.h>
#include <stdint.h>

#include <string.h>
#include <unistd.h>

/// \addtogroup server
/// @{

//============================================================================
//Typendeklaration

typedef struct
{
	int32_t head;
	int32_t tail;
	int32_t count;
	int32_t	maxElem;
	bool 	error;
}queueParam_t;

//============================================================================
//Variablendeklaration

static queueParam_t queue1Param;			//beinhaltet die Parameter rund um die Queue

static uint16_t* queue1[MAX_QUEUE1_ELEMENTS];		//bildet die eigentliche Queue
static uint16_t  memPerElement[MAX_QUEUE1_ELEMENTS][5*MAX_NUM_PIX];		//bildet die eigentliche Queue

//============================================================================
//Prototypen Funktionen


//============================================================================
//Implementation interne Funktionen

void cleanQueue_intern(void)	//Queue leeren für interne Anwendungen
{
	int i = 0;	//Laufvariable

//FIFO-Werte Null setzen
	for(i = (MAX_QUEUE1_ELEMENTS-1);i >= 0;i--)
	{
			queue1[i] = &memPerElement[i][0];
	}
}

//---------------------------------------------------------------------------

bool queueError(void)
{
	return queue1Param.error;
}

//---------------------------------------------------------------------------

int32_t succ(int32_t arg)
{
	return ((arg+1)%queue1Param.maxElem);
}

//============================================================================
//Implementation Funktionen

void queueInit(void)
{
	queue1Param.maxElem = MAX_QUEUE1_ELEMENTS;
	queue1Param.head = -1;
	queue1Param.tail = -1;
	queue1Param.count = 0;
	queue1Param.error = false;

	cleanQueue_intern();
}

//---------------------------------------------------------------------------

bool enqueue(uint16_t** part, int sizeArray)
{
	//queue1[queue1Param.tail] = *part;
	bool retval = false;

	if((((queue1Param.head+1) % queue1Param.maxElem) == queue1Param.tail) && (queue1Param.count==0)){
		//memcpy( (void*)(&memPerElement[queue1Param.tail][0]), (void*)(*part), MAX_NUM_PIX*2*sizeof(uint16_t));		//sho 25.10.17
		queue1Param.tail = succ(queue1Param.tail);
		queue1Param.head = succ(queue1Param.head);
		queue1Param.count++;
		//müsste den richtigen pointer wählen
	}else if(isQueueEmpty()){
		queue1Param.tail=0;
		queue1Param.head=0;
		queue1Param.count=1;
		retval=true;
	}else{
		queue1Param.head = succ(queue1Param.head);
		queue1Param.count++;
		retval=true;
	};

	memcpy( (void*)(&memPerElement[queue1Param.head][0]), (void*)(*part), sizeArray);		//sho 25.10.17

	return retval;
}

//---------------------------------------------------------------------------

bool dequeue(uint16_t** part)
{
		bool retVal = false;

		if(isQueueEmpty()){
			retVal = false;
		}else if(queue1Param.head == queue1Param.tail){
			*part = queue1[queue1Param.tail];
			queue1Param.head = -1;
			queue1Param.tail = -1;
			queue1Param.count--;
			retVal = true;
		}else{
			*part = queue1[queue1Param.tail];
			queue1Param.tail = succ(queue1Param.tail);

			if(--queue1Param.count < 0)
				queue1Param.count = 0;

			retVal = true;
		}

		return retVal;
}

//---------------------------------------------------------------------------
/*
bool isQueueFull(void)	//nur für externe Anwendung
{
	bool retVal = false;

	retVal = isQueueFull_intern();

	return retVal;
}*/

//---------------------------------------------------------------------------

bool isQueueEmpty(void)	//nur für externe Anwendung
{
	bool retVal = false;

	if((queue1Param.head) == -1 &&  (queue1Param.tail == -1))
		retVal = true;
	else
		retVal = false;

	return retVal;
}


/// @}
