/*----------------------------------------------------------------------------

 Copyright 2025, GHJ Morsink


 Author: MorsinkG

   Purpose:
      Implements measurement

   Contains:

   Module:

------------------------------------------------------------------------------
*/
/***------------------------- Includes ----------------------------------***/
#include <avr/io.h>
#ifndef _lint
#include <avr/interrupt.h>
#endif
#include "measure.h"
#include "timer.h"                      /* for time-out counter */
//#include "log.h"

/***------------------------- Defines -----------------------------------***/

#define STAB0TIME    8                  /* wait time in 10ms units for first measurement */
#define STABNTIME    4                  /* wait time in 10ms units for next measurements */
#define TIMEOUTADC   2                  /* timeout for ADC conversion */

/*  Flags for measurement */
#define NREADY       1                  /* not ready */
#define MODE_N       2                  /* positive quadrant */
#define FIRSTPH      4                  /* first phase measurement */
#define TIMOUT       8                  /* time out not completed */

#define ADSETTING    0xDF               /* set divider=128, ADIE=1, ADEN=1 ADIF=1, start conversion */

#define N_AD         2                  /*  number of ad samples for a measuremnt */

/***----------------------- Local Types ---------------------------------***/

/***------------------------- Local Data --------------------------------***/

static uint8_t  uFlags;

static uint8_t  AMeas0[N_AD];           /* storage for ADC analog measurements */
static uint8_t  AMeas1[N_AD];
static uint8_t  AMeas2[N_AD];
static uint8_t  AMeas3[N_AD];
static uint8_t  AMeas6[N_AD];
static uint8_t  aindex;

static uint8_t measure_delta;
static uint8_t measurecount;
static uint8_t repeatcount = 0;

/***------------------------ Global Data --------------------------------***/
uint8_t    numberofpoints;   /* number of measurement points */
uint8_t    bias;             /* current voltage on bias line */
uint8_t    side;             /* setting which quadrant ( NPN = 0, PNP = 1) */

/***------------------------ Local functions ----------------------------***/
/*--------------------------------------------------
 Start a new meas.point
 --------------------------------------------------*/
static void vStartNextMeasurement( uint8_t stabletime )
{
   int16_t    newOcr;

   if ( measurecount < numberofpoints )
   {
      vInitTimer();                       /* startup timer */
      uCountDownTimer = stabletime;       /* time-out of 70ms/30ms for getting to first level */
      TCNT1 = 0;                           /* clear counter */
      if (side == 1)
      {
         newOcr = OCR1A - measure_delta;
          if ( newOcr < 1)
          {
             newOcr = 1;
          }
      }
      else
      {
         newOcr = OCR1A + measure_delta;
         if ( newOcr > 0xFF)
         {
            newOcr = 0xFF;
         }
      }
      OCR1A = newOcr;                     /* 8 bit resolution set the first/next measurement voltage point */
      measurecount += 1;                  /*  next */
      uFlags = NREADY + FIRSTPH + TIMOUT + MODE_N;
      repeatcount = 0;
      aindex = 0;
      /*--------------------------------------------------
      wait for timeout to get first measurement, and start next
      --------------------------------------------------*/
   }
   else
   {
      measurecount = 0;
      uFlags = 0;
   }
}

/***------------------------ Global functions ---------------------------***/
/*--------------------------------------------------
 Start of measurement
 --------------------------------------------------*/
void vInitNPmeasurement( void )
{
   PORTB |= 0x08;    /*  pb3 = 1 */
   TCCR1A = 0xa1;                       /* fast 8bit pwm on OC1A and OC1B */
   TCCR1B = 0x09;                       /* fast 8bit pwm; max clock in (no division) */
   TCNT1 = 0;                           /* clear counter */
   // Set duty cycle for OC1A (0%):
   if ( side == 1 )                     /* get the npn/pnp function */
   {
      OCR1A = 255; // Adjust value between 0 (0%) and 255 (100%)
      OCR1B = 256 - bias;               /* 8 bit resolution */
      PORTC |= 0x10;
   }
   else
   {
      OCR1A = 1; // Adjust value between 0 (0%) and 255 (100%)
      OCR1B = bias;                     /* 8 bit resolution */
      PORTC &= ~(0x10);
   }
   measurecount = 0;
   measure_delta = 256/numberofpoints;
   PORTB &= ~(0x08);                    /* start pb3=0 */
   vStartNextMeasurement(STAB0TIME);    /* start with a delay of 150ms to stabilize DA */
}

/*--------------------------------------------------
 Make a means value
 --------------------------------------------------*/
uint8_t CalculateMean(uint8_t *Marray)
{
   uint16_t    Total = 0;
   uint8_t     count;

   for (count = 0; count < N_AD; count++)
   {
      Total += Marray[count];
   }
   Total = (Total + (N_AD/2)) / N_AD;

   return (Total & 0xFF);

}
/*--------------------------------------------------
 Check measurement completed
 --------------------------------------------------*/
void vCheckMeasurement( void )
{
   uint8_t  MeanValue, MeanValue2;

   repeatcount += 1;
   vLogInfo(PSTR("Check"));
   if ( (uFlags & NREADY) == 0 )
   {
      auCommandBuffer[0] = measurecount | 0x80;            /* We have a result */
      auCommandBuffer[1] = CalculateMean(AMeas2);          /* collector  */
      auCommandBuffer[2] = CalculateMean(AMeas0);          /* current */
      MeanValue2 = CalculateMean(AMeas1); /*(emitter)*/
      MeanValue = CalculateMean(AMeas3);
      auCommandBuffer[3] = (MeanValue - MeanValue2) & 0xff; /* base voltage */
      auCommandBuffer[4] = CalculateMean(AMeas6);           /* power voltage */
      auCommandBuffer[5] = MeanValue2;                      /* emitter */

      vStartNextMeasurement(STABNTIME);
   }
   else
   {
      vSet0Reply();
      if ( repeatcount > 4 )
      {
         measurecount -= 1;
         vStartNextMeasurement(STABNTIME);
         repeatcount = 0;
      }
   }
}

/*--------------------------------------------------
 Action on timeout
 --------------------------------------------------*/
void vTimeoutMeasurement( void )
{
   /* Lets check if this is a time-out of importance */
   if ( (uFlags & NREADY) == 0 )        /* do we have a result? */
   {
      TIMSK &= ~0x01;                   /* disable overflow interrupt */
      return;                           /* yes, nothing to do */
   }
   if ( (uFlags & FIRSTPH) == 0 )       /* first phase done: we can get full data */
   {
      uFlags &= ~NREADY;                /* ready */
      TIMSK &= ~0x01;                   /* disable overflow interrupt */
      return;
   }
   /* first stabilisation time is done, set new timeout and start analog measurement */
   uFlags &= ~FIRSTPH;
   ADMUX = 0x20;                        /* first channel, ADLAR=1;VREF IS REFERENCE; */
   ADCSRA = ADSETTING;                  /* start single measurement 1<<ADPS2 | 1<<ADPS1 | 1<<ADPS0 | 1<<ADEN | 1<<ADSC; ADIE, ADIF */
   uCountDownTimer = TIMEOUTADC;        /* 20 ms to complete */
   aindex = 0;
   PORTB |= 0x08;    /*  pb3 = 1 */
}

/*--------------------------------------------------
 analog input interrupt vector
 --------------------------------------------------*/
#ifdef _lint
void ISR_ADC_vect( void )
#else
ISR( ADC_vect )
#endif
{
   register uint8_t  uChannel = ADMUX & 0x07;

   ADCSRA = 0x90;                       /* clear the interrupt flag, disable interrupt and ADC conversion */
   switch ( uChannel )
   {
      case 0x00 :                       /* current measurement */
         AMeas0[aindex] = ADCH;
         ADMUX = 0x21;                  /* next channel */
         ADCSRA = ADSETTING;
         break;
      case 0x01 :                       /* emitter voltage (correction value) */
         AMeas1[aindex] = ADCH;
         ADMUX = 0x22;                  /* next channel */
         ADCSRA = ADSETTING;
         break;
      case 0x02 :                       /* collector voltage */
         AMeas2[aindex] = ADCH;
         ADMUX = 0x23;                  /* next channel */
         ADCSRA = ADSETTING;
         break;
      case 0x03 :                       /* base/gate voltage */
         AMeas3[aindex] = ADCH;
         ADMUX = 0x26;
         ADCSRA = ADSETTING;
         break;
      case 0x06:
         AMeas6[aindex] = ADCH;        /* voltage at powerside */

         /*lint -fallthrough */
      default:
         aindex++;
         if (aindex < N_AD)
         {
            ADMUX = 0x20;                        /* first channel, ADLAR=1;VREF IS REFERENCE; */
            ADCSRA = ADSETTING;                  /* start single measurement 1<<ADPS2 | 1<<ADPS1 | 1<<ADPS0 | 1<<ADEN | 1<<ADSC; ADIE, ADIF */
         }
         else
         {
            uFlags &= ~NREADY;                     /* ready */
            TIMSK &= ~0x01;                        /* disable overflow interrupt */
            PORTB &= ~0x08;    /* flip pb3 */
         }
         break;
   }
}

/* EOF */
