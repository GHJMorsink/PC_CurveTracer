/*----------------------------------------------------------------------------

 Copyright 2015-2025, GHJ Morsink


 Author: MorsinkG

   Purpose:
      Implements measurement function

   Contains:

   Module:
      main

------------------------------------------------------------------------------
*/
#include <stdint.h>

#ifndef MEASURE_H_
#define MEASURE_H_

#define REPORT_COUNT 6                  /* reports only contain 6 bytes as given here: */

/* ------------------------------------------------------------------------- */
/* ------------------- Application specific declarations ------------------- */
/* ------------------------------------------------------------------------- */
/*
COMMANDS RECEIVED FROM HOST
 Command bytes:   0   1   2   3    4    5    6
                  CC  XX  YY  ZZ   SS
 Command structure:
    CC: bit7   :	=0
		bit6..3:	Mode (Defines application)
        bit2..0:	Command (Defines application specific commands)
    XX, YY, ZZ, SS :	Command parameters (Defines command specific parameters)

 Commands:
    Application        |     Mode    Command     XX      YY      ZZ    SS
    ------------------------------------------------------------------
    MeasureN / both    |     0/1     0: Init     bias   count    side  0       0   0
                       |     0/1     1: start    0/bias  0       0
                       |     0/1     2: Poll     0       0       0
    Stop applications  |     15      0: Stop     0       0       0
    ------------------------------------------------------------------
   Considering the command structure, 13 new application can be realized (Mode 2..14)


ANSWERS SENT TO HOST
 Answer bytes: 0   1   2   3     4    5    6
                AA  XX  YY  ZZ   SS
 Answer structure:
    AA: bit7   :  Result Valid flag
      bit6..3  : Received Mode (or appl specific)
        bit2..0:	Received Command (Defines application specific commands)
    XX, YY, ZZ :	Answer parameters (Defines answer specific parameters)

 Answers:
    Application        |     Mode    Command     RV		XX      YY      ZZ
    -------------------------------------------------------------------------
    MeasureN both      |     0/1     0: Init     0    0       0       0
                       |     0/1     1: Start    0    0       0       0
                       |     0/1     2: Poll     1/0  V       I       bias
    Stop applications  |     15      0: Stop     -    -       -       -
    -------------------------------------------------------------------------
*/

#define MODEMEASUREN             0  // Command: 0x00 0xXX 0xZZ 0xZZ     (XX: subcommand)
#define MODEMEASUREBOTH          1  // Command: 0x01 0xXX 0xZZ 0xZZ     (XX: subcommand)
#define MODESTOP                 15 // Command: 0xff 0x00 0x00 0x00
#define CMD_INIT                 0
#define CMD_START                1
#define CMD_POLL                 2
#define CMD_CALIBRATE            3
#define CMD_RCAL                 4


#ifndef vDebugHex
#define vDebugHex(a,b,c)      ((void)(0))
#define vLogInfo(a)           ((void)(0))
#endif

/* defined in Curvetracer.c : */
extern uint8_t    uStartInit;                        /* measurement start command */
extern uint8_t    auCommandBuffer[REPORT_COUNT];     /* responsebuffer */

extern uint8_t    numberofpoints;   /* number of measurement points; should be exponent of 2 */
extern uint8_t    bias;             /* current voltage on bias line */
extern uint8_t    side;             /* setting which quadrant (pos NPN = 0, or neg PNP = 0xff) */


extern void vSet0Reply( void );

/*--------------------------------------------------
 Start of measurement capacitance
 --------------------------------------------------*/
extern void vInitNPmeasurement( void );

/*--------------------------------------------------
 Action on timeouts
 --------------------------------------------------*/
extern void vTimeoutMeasurement( void );

/*--------------------------------------------------
 Check measurement completed
 --------------------------------------------------*/
extern void vCheckMeasurement( void );

#endif

