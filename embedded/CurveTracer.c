/*----------------------------------------------------------------------------

 Copyright 2025, GHJ Morsink


 Author: MorsinkG

   Purpose:
      Implements main selection function on basis of USB HID device

   Contains:

   Module:
      main

------------------------------------------------------------------------------
*/
/***------------------------- Includes ----------------------------------***/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <string.h>
#include "usbdrv.h"
#include "measure.h"
#include "timer.h"

/***------------------------- Defines -----------------------------------***/



/***----------------------- Local Types ---------------------------------***/

PROGMEM const unsigned char usbHidReportDescriptor[22] = {    /* USB report descriptor */
    0x06, 0x00, 0xff,              // USAGE_PAGE (Generic Desktop)
    0x09, 0x01,                    // USAGE (Vendor Usage 1)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00,              //   LOGICAL_MAXIMUM (255)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x95, REPORT_COUNT,            //   REPORT_COUNT (4)
    0x09, 0x00,                    //   USAGE (Undefined)
    0xb2, 0x02, 0x01,              //   FEATURE (Data,Var,Abs,Buf)
    0xc0                           // END_COLLECTION
};
/* Since we define only one feature report, we don't use report-IDs (which
 * would be the first byte of the report). The entire report consists of
 * opaque data bytes.
 */


/***------------------------- Local Data --------------------------------***/
/* The following variables store the status of the current data transfer */
static uint8_t       uCurrentAddress;
static uint8_t       uBytesRemaining;
uint8_t              auInternalCommand[REPORT_COUNT+1];
uint8_t              reportBuffer[4];
/* Variables */

/***------------------------ Global Data --------------------------------***/

uint8_t              uStartInit;
uint8_t              Mode;
uint8_t              auCommandBuffer[REPORT_COUNT];  /* response buffer */

#ifdef _lint
void wdt_init(void);
static uint8_t    uMCUSRcopy;

#define  WDT_RESET()
#define  WDT_ENABLE(time)  ((void) 0)
#define  WDT_DISABLE()

#else
void wdt_init(void) __attribute__((naked)) __attribute__((section(".init3")));      /* reset WDT after soft reset */
static uint8_t    uMCUSRcopy __attribute__ ((section (".noinit")));

#define  WDT_RESET()       wdt_reset()
#define  WDT_ENABLE(time)  wdt_enable(time)
#define  WDT_DISABLE()     wdt_disable()

#endif

/***------------------------ Local functions ----------------------------***/

/*--------------------------------------------------
 Check which function/mode is running; and act on it
 This starts initialisation in the main loop (not within the USB interrupt system)
 --------------------------------------------------*/
static void vCheckUsbMode( void )
{
   switch ( uStartInit )
   {
      case (MODEMEASUREN + 1):
         vInitNPmeasurement();
         uStartInit = 0;
         break;
      case (MODEMEASUREBOTH + 1):
         // todo vInitBmeasurement()
         uStartInit = 0;
         break;
      // no other functions yet
      default:
         break;
   }
}

/*--------------------------------------------------
 Setting data to the EEprom for calibration:
 Function '0' has 8 bytes, uses 0,1,2,3
 Function '1' has 8 bytes, uses 8,9
 etc.
 --------------------------------------------------*/
static void vSetCal(uint8_t uFunction, uint8_t *auCommand )
{
   uint16_t uBaseAddress;

   uBaseAddress = uFunction << 3;                           /* Set the base address in eeprom as 8 times the function */
   uBaseAddress += (auCommand[1] << 1);                     /* Plus the offset as given for words */
   eeprom_write_byte( (uint8_t *)uBaseAddress, auCommand[2] );        /* store it,  */
   eeprom_write_byte( (uint8_t *)(uBaseAddress + 1), auCommand[3] );  /* store it,  */
}

/*--------------------------------------------------
 Reading calibration data
 --------------------------------------------------*/
static void vReadCal( uint8_t uFunction, uint8_t *auCommand )
{
   uint16_t uBaseAddress;

   uBaseAddress = uFunction << 3;                           /* Set the base address in eeprom as 8 times the function */
   uBaseAddress += (auCommand[1] << 1);                     /* Plus the offset as given for words */
   auCommand[2] = eeprom_read_byte( (uint8_t *)uBaseAddress );        /* read it,  */
   auCommand[3] = eeprom_read_byte( (uint8_t *)(uBaseAddress + 1) );  /* read it,  */
   auCommand[0] |= 0x80;              /* We have a result */
}

/*--------------------------------------------------
 Port initialization
 --------------------------------------------------*/
static void vPortInit( void )
{
   uint8_t i, j;

   DDRB = 0x2F;                        /* output PB0, 1, 2, 3, 5 */
   PORTB = 0x00;                       /* all off */
   DDRC = 0x10;                        /* port C all inputs except PC4 */
   PORTC = 0x00;                       /* inputs, pc4 output = 0 */
   SFIOR = PUD;                        /* disable pullups on all ports */
   TCCR0 = 0;                          /* Timer0 clock= no clock source */
   TCCR1B = 0;                         /* Stop Timer1 */
   ADCSRA = 0;
   ADMUX = 0;

   PORTD = 0xe0;                       /* 1110 0000 bin: activate pull-ups except on USB lines */
   DDRD = 0x16;                        /* 0001 0110 bin: all pins input except USB (-> USB reset) */
   j = 0;
   while(--j)                          /* USB Reset by device only required on Watchdog Reset */
   {
      i = 0;
      while(--i) ;                     /* delay >10ms for USB reset */
   }
   DDRD = 0x02;                        /* 0000 0010 bin: remove USB reset condition */
   /* configure timer 0 for a rate of 12M/(1024 * 256) = 45.78 Hz (~22ms) */
   TCCR0 = 5;                          /* timer 0 prescaler: 1024 */

}


/*--------------------------------------------------
 Set the reply to a command (zero)
 --------------------------------------------------*/
void vSet0Reply( void )
{
   auCommandBuffer[1] = 0;
   auCommandBuffer[2] = 0;
   auCommandBuffer[3] = 0;
   auCommandBuffer[4] = 0;
   auCommandBuffer[5] = 0;
}


/*--------------------------------------------------
 Interpret the command given from USB interface
 --------------------------------------------------*/
void vInterpretCommands( uint8_t * auCommand )
{
   uint8_t  uMode;
   uint8_t  uCmd;

   uCmd = auCommand[0] & 0x07;
   uMode = (auCommand[0] >> 3) & 0x0F;
   vDebugHex( PSTR("Mode Interpreter"), &uMode, 1);
   vDebugHex( PSTR("\nCmd Interpreter"), &uCmd, 1);
   PORTB ^= 0x01;    /* flip LED */
   switch ( uMode )
   {
      case MODEMEASUREN:
         switch ( uCmd )
         {
            case CMD_INIT:
               uStartInit = (MODEMEASUREN + 1);
               bias = auCommand[1];
               numberofpoints = auCommand[2];
               side = auCommand[3];
               vSet0Reply();
               break;
            case CMD_POLL:
               vCheckMeasurement();  /* get the current status */
               break;
            case CMD_CALIBRATE :
               vSetCal( 0, auCommand );
               break;
            case CMD_RCAL :
               vReadCal( 0, auCommand );
               break;
            default:
               break;
         }
         break;
#if 0 //Not implemented
      case MODEMEASUREBOTH:
         switch ( uCmd )
         {
            case CMD_INIT:
               uStartInit = (MODEMEASUREBOTH + 1);
               vSet0Reply();
               break;
            case CMD_POLL:
               vCheckMeasurement();  /* get the current status */
               break;
            case CMD_CALIBRATE :
               vSetCal( 1, auCommand );
               break;
            case CMD_RCAL :
               vReadCal( 1, auCommand );
               break;
            default:
               break;
         }
         break;
#endif
      case MODESTOP:                    /* Request for reboot */
         WDT_ENABLE( WDTO_15MS );
         vLogInfo(PSTR("Rebooting"));
         for(;;)
         {
            ;
         }
         break;                       /*lint !e527 (lint says 'unreachable'; cppcheck wants a 'break') */
      default:
         break;
   }
}


/***------------------------ Global functions ---------------------------***/


/*--------------------------------------------------
 The entry point  (watchdog-init and resetinfo already done)
 --------------------------------------------------*/
__attribute__((OS_main)) int main(void)
{
   cli();
   wdt_disable();                /* disable the watchdog */
   vPortInit();
   vInitTimer();
   usbInit();

   sei();
   if ( (uMCUSRcopy & 0x1E ) != 0x1E )     /* show resets */
   {
      vDebugHex( PSTR("Reset occurred"), &uMCUSRcopy, 1 );
   }
   vLogInfo(PSTR( "Starting"));
   uStartInit = 0;

   for(;;)
   {
      WDT_RESET();                     /* Reset the watchdog */
      usbPoll();                       /* Check the USB line */
      vCheckUsbMode();                 /* Check our function */
   }
}

/*--------------------------------------------------
 Setup USB transaction
 --------------------------------------------------*/
usbMsgLen_t usbFunctionSetup(uint8_t data[8])
{
   usbRequest_t    *rq = (void *)data;

   //vDebugHex( PSTR("Request"), data, 8);
   if ( ( (rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS ) &&                              /* HID class request */
        ( ( rq->bRequest == USBRQ_HID_GET_REPORT ) || ( rq->bRequest == USBRQ_HID_SET_REPORT ) ) )    /* ReportType (highbyte), ReportID (lowbyte) */
   {
      /* since we have only one report type, we can ignore the report-ID */
      uBytesRemaining = REPORT_COUNT;
      uCurrentAddress = 0;
      return USB_NO_MSG;  /* use usbFunctionRead() to obtain data  or
                             use usbFunctionWrite() to receive data from host */
   }
   return 0;
}

/*--------------------------------------------------
 usbFunctionRead() is called when the host requests a chunk of data from
 the device. For more information see the documentation in usbdrv.h.
 --------------------------------------------------*/
uint8_t usbFunctionRead( uint8_t *data, uint8_t len )
{
   vDebugHex(PSTR("auCmd"), auCommandBuffer, REPORT_COUNT);
   if ( len > uBytesRemaining )
   {
      len = uBytesRemaining;
   }
   memcpy (data, auCommandBuffer + uCurrentAddress, len);
   uCurrentAddress += len;
   uBytesRemaining -= len;
   return len;
}

/*--------------------------------------------------
 usbFunctionWrite() is called when the host sends a chunk of data to the
 device. For more information see the documentation in usbdrv.h.
 --------------------------------------------------*/
uint8_t   usbFunctionWrite( uint8_t *data, uint8_t len )
{
   //vDebugHex(PSTR("\nFWrite"), data, REPORT_COUNT);
   if ( uBytesRemaining == 0 )
   {
      return 1;               /* end of transfer */
   }
   if ( len > uBytesRemaining )
   {
      len = uBytesRemaining;
   }
   if ( (uCurrentAddress + len) > REPORT_COUNT )
   {
      return 1;
   }
   memcpy ( auCommandBuffer + uCurrentAddress, data, len );
   uCurrentAddress += len;
   uBytesRemaining -= len;

   vDebugHex(PSTR("Bytes"), auCommandBuffer, REPORT_COUNT);
   vInterpretCommands( auCommandBuffer );       /* translate the given command */
   return uBytesRemaining == 0;                 /* return 1 if this was the last chunk (normal in this application) */
}


/* ------------------------------------------------------------------------- */
// reset WDT called before main()
void wdt_init(void)
{
   uMCUSRcopy = MCUSR;                  /* keep data */
   MCUSR = 0;
   WDT_DISABLE();
}


/* EOF */
