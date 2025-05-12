/*----------------------------------------------------------------------------

 Copyright 2025, GHJ Morsink


 Author: MorsinkG

   Purpose:
      Implements general timer functions

   Contains:

   Module:

------------------------------------------------------------------------------
*/

/***------------------------- Defines ------------------------------------***/

/***------------------------- Includes ----------------------------------***/

#include <avr/io.h>
#include <avr/interrupt.h>

#include "timer.h"
#include "measure.h"

/***------------------------- Types -------------------------------------***/

/***----------------------- Local Types ---------------------------------***/

/***------------------------- Local Data --------------------------------***/

uint8_t     uCountDownTimer;
uint8_t     uCurrentTimeConstant;

/***------------------------ Global Data --------------------------------***/

/***------------------------ Global functions ---------------------------***/
/*--------------------------------------------------
 Initialize hardware (interrupts are disabled)
 --------------------------------------------------*/
void vInitTimer( void )
{
   TCCR0  = 0x05;                       /* Prescaler 1024; CTC mode 0.0853 ms per count */
   TCNT0  = 256 - TEN_MS;                 /* count starting at -58: every 5ms timer-overflow 117: 10ms*/
   TIMSK |= 0x01;                       /* At overflow enable interrupt */

   uCountDownTimer = 0;
}


/***------------------------ Interrupt functions ------------------------***/
/*--------------------------------------------------
 System clock
 Runs at 5/10/16 ms per tick (32 ticks per second)
 --------------------------------------------------*/
#ifdef _lint
void TIMER0_OVF_vect( void )
#else
ISR(TIMER0_OVF_vect)
#endif
{
   TCNT0  = 256 - TEN_MS;   /* count starting at -58: every 5ms timer-overflow */
   if ( uCountDownTimer != 0 )
   {
      uCountDownTimer--;
      if ( uCountDownTimer == 0 )       /* time elapsed: run specific commands */
      {
         vTimeoutMeasurement();
      }
   }
}

/* EOF */
