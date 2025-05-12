/*----------------------------------------------------------------------------

 Copyright 2015, GHJ Morsink

 Author: MorsinkG

   Purpose:
      Definitions for the bootloader on the ATMega8 board

   Contains:

   Module:


------------------------------------------------------------------------------
*/
#ifndef DEFS_H_
#define DEFS_H_

#define _ATMEGA8   // device select: _ATMEGAxxxx
#define _B1024   // boot size select: _Bxxxx (words), powers of two only

#define TEST_MODE 0

#include	<avr/io.h>


/* baud rate register value calculation */
#define CPU_FREQ                12000000L
#define BAUD_RATE               38400

/* definitions for UART control */
#define BAUD_RATE_LOW_REG       UBRR0L
#define UART_CONTROL_REG        UCSR0B
#define ENABLE_TRANSMITTER_BIT  TXEN0
#define ENABLE_RECEIVER_BIT     RXEN0
#define UART_STATUS_REG         UCSR0A
#define TRANSMIT_COMPLETE_BIT   TXC0
#define RECEIVE_COMPLETE_BIT    RXC0
#define UART_DATA_REG           UDR0

/* definitions for SPM control */
#define SPMCR_REG               SPMCR
#define PAGESIZE                64     /* 32 words */
#define APP_END                 6144
//#define	LARGE_MEMORY

/* definitions for device recognition */
#define PARTCODE                0x77
#define SIGNATURE_BYTE_1        0x1E
#define SIGNATURE_BYTE_2        0x93
#define SIGNATURE_BYTE_3        0x0F

/* definitions for locations */
#define OKLOCATION               (0x100)  /* location in ATMega8 where the code should reside */
#define APPL_START               0x0000

/* standard characters */
#define ACK                      0x06
#define NAK                      0x15
#define CR                       0x0D
#define LF                       0x0A
#define BOOTVERSION              ("V1.00")
#define BOOTVLEN                 5     /* length of bootversion string */

/* Result codes */
//#define RESULT_SUCCESS           0
//#define RESULT_ERROR             1
//#define RESULT_END               2

#endif                                  /* DEFS_H_ */

/* EOF */
