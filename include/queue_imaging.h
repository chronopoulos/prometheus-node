/**	Dieses Modul beinhaltet jediglich ein FIFO. Die Groesse
 * 	des FIFOS wird im DEF/DEF_systemSettings unter
 * 	MAX_QUEUE1_ELEMENTS	bestimmt.
 *
 * @file	FWO_queue.h
 *
 *
 *
 * @author	S. Honegger
 *
 * @date	20.05.2015
 *
 * @version	1.2
 */

//============================================================================
//defines

#ifndef QUEUE_IMAGING_H_
#define QUEUE_IMAGING_H_

//---------------------------------------------------------------------------
//QUEUE fuer die anzufahrende XY-Positionen

#define MAX_QUEUE1_ELEMENTS		4


//============================================================================
//includes

#include <stdint.h>
#include <stdbool.h>

//============================================================================
//Prototypen Funktionen

/**	Initialisierung ein FIFO. Setzt die FIFO-Werte auf Null.
 *
 */
void queueInit();

/**	Legt den Wert auf der Adresse des uebergebenen Pointers in die Queue.
 *
 * @param	part  		Der uebergebende Wert auf der Adresse des uebergebenen Pointers.
 *
 * @return	true/false 	Falls das FIFO voll war, wird ein false zurueckgegeben.
 */
bool enqueue(uint16_t** part,int sizeArray);

/**	Nimmt nach dem FIFO Prinzip den Wert aus der Queue.
 *
 * @param	part 		Die Adresse auf den der Pointer zeigt, beinhaltet den geholten Wert.
 *
 * @return	true/false	Falls das FIFO leer war, wird ein false zurueckgegeben.
 */
bool dequeue(uint16_t** part);

/** Ueberprueft ob die Queue leer ist.
 *
 * @return	true/false
 */
bool isQueueEmpty();


/** Ueberprueft ob die Queue voll ist.
 *
 * @return	true/false
 */
//bool isQueueFull();

#endif /* QUEUE_IMAGING_H_ */
