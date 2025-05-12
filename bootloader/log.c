/*----------------------------------------------------------------------------

 Copyright 2013, GHJ Morsink


 Author: MorsinkG

   Purpose:
      Implements log function

   Contains:

   Module:

------------------------------------------------------------------------------
*/
#include "defines.h"

#if TEST_MODE

#include "serial.h"
#include <string.h>
#include <avr/pgmspace.h>
#include "log.h"

#define CHARTABLE    0                  /* use code conversion */
#define MAX_STRING   40                 /* Length of buffer in uart routines */

/*----------------------------------------------------------------------
    vByteToHex

    Set string of two hex-characters from a byte
----------------------------------------------------------------------*/
#if CHARTABLE
static const char *acHex = "0123456789ABCDEF";
#define cHex(c)      ((unsigned char) acHex[c])         /* use charactertable */

#else
static unsigned char cHex( unsigned char cRef )  /* use code */
{
   register unsigned char uRet;

   uRet = cRef + '0';
   if ( uRet > '9' )
   {
      uRet += 7;
   }
   return uRet;
}
#endif


/*****************************************************************************
   Function :  NA_WriteBuffer

   this function will try to write the contents of a buffer to a port

   In :  port_h         Handle to the PORT
         pbOut          pointer to buffer with data to send
         size           size of the input buffer
         pCount         pointer to store the  count of bytes actually sent

   Out : -

   Returns :   NASuccess  buffer sent completely
               NAFailure  error occurred (no port)
*****************************************************************************/
void NA_WriteBuffer( unsigned char *pbOut, const unsigned char size )
{
    unsigned char   uSentCount;

    for ( uSentCount = 0; uSentCount < size; uSentCount++ )
    {

      vSerialPutChar( pbOut[ uSentCount ] );                        /* send 1 character */
    }
    return;
}

/*----------------------------------------------------------------------
    vDebugHex

      For debug messages: send first string, followed by hex representation second string with iLen
    No check is done on the serial-output buffer
----------------------------------------------------------------------*/
/*lint -e850 */
void vDebugHex( const prog_char *szHeader, unsigned char *acData, unsigned int iLen )
{
    unsigned int  iTotalLen, i;
    unsigned char acTotalString[ MAX_STRING ];
    unsigned char cData;

    iTotalLen = strlen_P( szHeader );
    if ( iTotalLen > (MAX_STRING - 11 ) )
    {
        return ;
    }
    memcpy_P( acTotalString, szHeader, iTotalLen );

    acTotalString[ iTotalLen ] = ':';
    iTotalLen += 1;

    for ( i = 0; i < iLen; i++ )
    {
        acTotalString[ iTotalLen ] = ' ';
        cData = (acData[ i ] & 0x00F0) >> 4;
        acTotalString[ iTotalLen + 1 ] = cHex(cData);
        cData = acData [ i ] & 0x000F;
        acTotalString[ iTotalLen + 2 ] = cHex(cData);
        iTotalLen += 3;
        if ( iTotalLen > ( MAX_STRING - 5 ) )  /* protect against 'out of boundary' */
        {
            i = iLen;
        }
    }
    acTotalString[ iTotalLen ] = '\r';
    iTotalLen += 1;
    acTotalString[ iTotalLen ] = '\n';
    iTotalLen += 1;

    NA_WriteBuffer( acTotalString, iTotalLen );
}

/*----------------------------------------------------------------------
    vLogInfo

      For service messages: send first string
----------------------------------------------------------------------*/
void vLogInfo( const prog_char *szHeader )
{
    unsigned int  iTotalLen;
    unsigned char acTotalString[ MAX_STRING ];

    iTotalLen = strlen_P( szHeader );
    if ( iTotalLen > (MAX_STRING - 2) )
    {
        iTotalLen = MAX_STRING - 2;
    }
    memcpy_P( acTotalString, szHeader, iTotalLen );
    acTotalString[ iTotalLen ] = '\r';
    iTotalLen += 1;
    acTotalString[ iTotalLen ] = '\n';
    iTotalLen += 1;
    NA_WriteBuffer( acTotalString, iTotalLen );
}

#endif                                  /* TEST_MODE */
/* EOF */
