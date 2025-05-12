/*----------------------------------------------------------------------------

 Copyright 2025, GHJ Morsink


 Author: MorsinkG

   Purpose:
      Implements general timer functions

   Contains:

   Module:
------------------------------------------------------------------------------
*/

#ifndef TIMER_H_
#define TIMER_H_

/***------------------------- Defines ------------------------------------***/

#define SIXTEEN_MS  187
#define TEN_MS      117
#define FIVE_MS     58
/***------------------------- Includes ----------------------------------***/
#include <stdint.h>

/***------------------------- Types -------------------------------------***/


/***------------------------ Global Data --------------------------------***/

/* The Overflow timer.
   This timer can be accessed without a semaphore: it is a single byte
   The value decrements every 5ms */
extern uint8_t     uCountDownTimer;              /* counting down time-out */

/***------------------------ Global functions ---------------------------***/

/*--------------------------------------------------
 Initialize hardware
 --------------------------------------------------*/
extern void vInitTimer( void );


#endif /* TIMER_H_ */
