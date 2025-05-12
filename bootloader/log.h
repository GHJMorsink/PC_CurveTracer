/*----------------------------------------------------------------------------

 Copyright 2013, GHJ Morsink


 Author: MorsinkG

   Purpose:
      Implements log function

   Contains:

   Module:

------------------------------------------------------------------------------
*/
#ifndef LOG_H_
#define LOG_H_

#include <avr/pgmspace.h>

typedef char PROGMEM prog_char;

/*--------------------------------------------------
 Send a string
 --------------------------------------------------*/
extern void NA_WriteBuffer( unsigned char *pbOut, const unsigned char size );

/*----------------------------------------------------------------------
    For debug messages: send first string, followed by hex representation second string with iLen
    No check is done on the serial-output buffer
----------------------------------------------------------------------*/
extern void vDebugHex( const prog_char *szHeader, unsigned char *acData, unsigned int iLen );

/*----------------------------------------------------------------------
      For service messages: send first string
----------------------------------------------------------------------*/
extern void vLogInfo( const prog_char *szHeader );



#endif /* LOG_H_ */

