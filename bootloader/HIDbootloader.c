/**----------------------------------------------------------------------------

 Copyright 2015, GHJ Morsink

 Author: MorsinkG

   Purpose:
      Implements bootloader

   Contains:

   Module:
      Bootloader HID for USB-LCRmeter, CurveTracer

Derived from:
 * Project: AVR bootloader HID
 * Author: Christian Starkjohann
 * Creation Date: 2007-03-19
 * Copyright: (c) 2007 by OBJECTIVE DEVELOPMENT Software GmbH
 * License: GNU GPL v2 (see License.txt)

------------------------------------------------------------------------------
*/

/***------------------------- Includes ----------------------------------***/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <avr/boot.h>

#include "usbconfig.h"
#include "usbdrv.h"
#include "defines.h"
#include "flash.h"


#if TEST_MODE
#include "serial.h"
#include "log.h"
#else
#define vLogInfo(a)        ((void)0)
#define vDebugHex(a,b,c)   ((void)0)
#define vSerialInit()      ((void)0)
#endif

/***------------------------- Defines ------------------------------------***/

//#define MAINTIMEOUT        0xF8000       /* timeout of ca. 5s (polling)/approx.  */
#define MAINTIMEOUT        0x7C000       /* timeout of ca. 5s (polling)/approx.  */

/***------------------------- Types -------------------------------------***/

/***----------------------- Local Types ---------------------------------***/

/***------------------------- Local Data --------------------------------***/

static uint8_t          offset;         /* data already processed in current transfer */
static uint8_t          uReceiveflag;

static uint8_t replyBuffer[7] =
{
      1,                              /* report ID */
      PAGESIZE & 0xff,
      PAGESIZE >> 8,
      ((long)FLASHEND + 1) & 0xff,
      (((long)FLASHEND + 1) >> 8) & 0xff,
      (((long)FLASHEND + 1) >> 16) & 0xff,
      0
};

const PROGMEM unsigned char usbHidReportDescriptor[33] = {
    0x06, 0x00, 0xff,              // USAGE_PAGE (Generic Desktop)
    0x09, 0x01,                    // USAGE (Vendor Usage 1)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00,              //   LOGICAL_MAXIMUM (255)
    0x75, 0x08,                    //   REPORT_SIZE (8)

    0x85, 0x01,                    //   REPORT_ID (1)
    0x95, 0x06,                    //   REPORT_COUNT (6)
    0x09, 0x00,                    //   USAGE (Undefined)
    0xb2, 0x02, 0x01,              //   FEATURE (Data,Var,Abs,Buf)

    0x85, 0x02,                    //   REPORT_ID (2)
    0x95, 0x43,                    //   REPORT_COUNT (67)
    0x09, 0x00,                    //   USAGE (Undefined)
    0xb2, 0x02, 0x01,              //   FEATURE (Data,Var,Abs,Buf)
    0xc0                           // END_COLLECTION
};


#define PINLOAD()    ((PINB & 0x08) == 0)   /* True if jumper on PB3 to GND is set */

/***------------------------ Global Data --------------------------------***/

/***------------------------ Local functions ----------------------------***/

/*--------------------------------------------------
 Reset to the application
 --------------------------------------------------*/
static void vLeaveBootloader( void )
{
   uint8_t  uCount;
   void  (*vAppl)(void) = APPL_START;

   if (PINLOAD())
   {
      return;                 /* requested to stay in bootload */
   }

   uCount = 0;
   while ( uCount < 128 )
   {
      if ( _LOAD_PROGRAM_MEMORY(OKLOCATION + uCount) != 0xFF )
      {
#if TEST_MODE
         vLogInfo(PSTR("Leaving.."));
         return;
#else
         cli();
         _ENABLE_RWW_SECTION();
         USB_INTR_ENABLE = 0;
         USB_INTR_CFG = 0;             /* also reset config bits */
         TCCR0 = 0;                    /* default value */
         GICR = (1 << IVCE);           /* enable change of interrupt vectors */
         GICR = 0;                     /* move interrupts to application flash section */
         vAppl();                      /* This leaves the bootloader ...  */
#endif
      }
      uCount++;
   }
}

/***------------------------ Global functions ---------------------------***/
/*--------------------------------------------------
 The USB function setup transmission
 --------------------------------------------------*/
uint8_t  usbFunctionSetup( uint8_t *data)
{
   usbRequest_t   *rq = (void *)data;

   uReceiveflag = 1;
   if ( rq->bRequest == USBRQ_HID_SET_REPORT )
   {
      vLogInfo( PSTR("New"));
      if ( rq->wValue.bytes[0] == 2 )   /* report id is '2'? Then this is a new starting point */
      {
         offset = 0;
         return USB_NO_MSG;
      }
   }
   else if( rq->bRequest == USBRQ_HID_GET_REPORT )
   {
      usbMsgPtr = (usbMsgPtr_t)replyBuffer;
      return 7;
   }
   return 0;
}

/*--------------------------------------------------
 Getting data via USB
 datablock as:
 3 bytes startaddress
 following all binary data (len - 4), should be PAGESIZE = 64
 --------------------------------------------------*/
uint8_t usbFunctionWrite( uint8_t *data, uint8_t len )
{
   static union {
      uint16_t    All;
      uint8_t     Byt[2];
   } sAddress;
   static uint16_t   uCurrentblock;     /* reference of the block in control */

   vDebugHex( PSTR("Rcv"), data, len);  /* We receive in chunks of 8 bytes */
   if ( offset == 0 )                   /* new address into the struct */
   {
      sAddress.Byt[0] = data[1];
      sAddress.Byt[1] = data[2];        /* get the complete address */
      data += 4;                        /* data starts at offset 4 */
      len -= 4;
      if ( sAddress.All == 0xFFC0 )
      {
         vLogInfo( PSTR("ending"));
         wdt_enable( WDTO_15MS );
         for (;;)
         {
            ;
         }
      }
   }
   offset += len;
   while ( len != 0 )
   {
      if ( (sAddress.Byt[0] & (SPM_PAGESIZE - 1)) == 0 )              /* if page start: erase */
      {
         uCurrentblock = sAddress.All;  /* keep the reference to this block */
#if TEST_MODE
         vDebugHex( PSTR("Strt"), (uint8_t *) &uCurrentblock, 2);
#else
         cli();
         _PAGE_ERASE( sAddress.All );   /* erase page */
         sei();
         _WAIT_FOR_SPM();       /* wait until page is erased */
#endif
      }
      cli();
      _FILL_TEMP_WORD(sAddress.All, *(uint16_t *)data);
      sei();
      sAddress.All += 2;
      data += 2;
      /* write page when we cross page boundary */
      if ( (sAddress.Byt[0] & (SPM_PAGESIZE - 1)) == 0 )
      {
#if TEST_MODE
#else
         cli();
         _PAGE_WRITE( uCurrentblock );  /* write to the given saved block reference */
         sei();
         _WAIT_FOR_SPM();
#endif
      }
      len -= 2;
   }
   if ( offset == PAGESIZE )
   {
        return offset;
   }
   return 0;
}

/*--------------------------------------------------
 Main
 --------------------------------------------------*/
int main( void )
{
   uint32_t  uMainTimeout;
   volatile uint8_t   i, j;

   /* initialize hardware */
   wdt_disable();
   DDRB = 0x21;                        /* output PB0, 5 */
   PORTB = 0x0C;                       /* pullup on PB3, LED on at PB0 */
#if TEST_MODE
   vSerialInit();
#else
   GICR = (1 << IVCE);  /* enable change of interrupt vectors */
   GICR = (1 << IVSEL); /* move interrupts to boot flash section */
#endif
   /* enforce USB re-enumerate: */
   DDRD = 0x17;                        /* 0001 0111 bin: all pins input except USB (-> USB reset) */
   j = 0;
   while(--j)                          /* USB Reset by device only required on Watchdog Reset */
   {
      i = 0;
      while(--i) ;                     /* delay >10ms for USB reset */
   }
   DDRD = 0x02;                        /* 0000 0010 bin: remove USB reset condition */
   sei();
   usbInit();

   for(;;)                              /* main loop */
   {
      vLogInfo( PSTR("Boot..") );
      uMainTimeout = MAINTIMEOUT;
      while ( uMainTimeout > 0 )
      {
         usbPoll();                     /* Get the incoming messages and reset the timeout */
         uMainTimeout -= 1;
         if ( uReceiveflag != 0 )
         {
            uReceiveflag = 0;
            uMainTimeout = MAINTIMEOUT;
         }
      }
      vLeaveBootloader();                /* No USB messages anymore, check if we can get to exit */
   }
}

/* EOF */
