/*----------------------------------------------------------------------------

 Copyright 2011, GHJ Morsink


 Author: MorsinkG

   Purpose:
      Implements serial function

   Contains:

   Module:
      main

------------------------------------------------------------------------------
*/
#include <stdint.h>

#ifndef SERIAL_H_
#define SERIAL_H_


#define RESULT_SUCCESS                        (0)
#define RESULT_ERROR                          (1)

extern uint8_t uRxOverflow;
extern uint8_t uTxOverflow;

void vSerialInit( void );                 /* Initialize UART and Flush FIFOs */
void vSerialPutChar( uint8_t );           /* Put a byte into UART Tx FIFO */
uint8_t uSerialGetChar( uint8_t *uRcv );  /* Get char from UART Rx FIFO but non blocking */

/*--------------------------------------------------
 Change baudrate setting; deliver baudrate in normal value (f.i. 9600, 19200, 38400)
 --------------------------------------------------*/
extern void vSetBaud( uint8_t uBaudrate );

/*--------------------------------------------------
 Check size of open locations in transmit buffer
 --------------------------------------------------*/
extern uint8_t uSerialGetFree( void );

#endif /* UART_H_ */

