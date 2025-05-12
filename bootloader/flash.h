/*----------------------------------------------------------------------------

 Copyright 2011, GHJ Morsink


 Author: MorsinkG

   Purpose:
      Implements flash interface for GCC (partly see Atmel AVR109 note)

   Contains:

   Module:

------------------------------------------------------------------------------
*/
#ifndef FLASH_H_
#define FLASH_H_

/* AVR-GCC/avr-libc */
#include <limits.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>

#if _lint

#define _ENABLE_RWW_SECTION()          {}
#define _WAIT_FOR_SPM()                {}
#  define _LOAD_PROGRAM_MEMORY(addr)   0
#  define _LOAD_PROGRAM_WORD(addr)     0
#define _FILL_TEMP_WORD(addr,data)     ((void)(0))
#define _PAGE_ERASE(addr)              ((void)(0))
#define _PAGE_WRITE(addr)              (void)(addr)
#define _EEPROM_WRITE(addr,data)       ((void)(0))
#define _EEPROM_READ(addr)             (0)

#else
#include <avr/boot.h>

#define _ENABLE_RWW_SECTION()          boot_rww_enable()
#define _WAIT_FOR_SPM()                boot_spm_busy_wait()

#ifndef LARGE_MEMORY
#  define _LOAD_PROGRAM_MEMORY(addr)   pgm_read_byte_near(addr)
#  define _LOAD_PROGRAM_WORD(addr)     pgm_read_word_near(addr)
#else /* LARGE_MEMORY */
#  define _LOAD_PROGRAM_MEMORY(addr)   pgm_read_byte_far(addr)
#  define _LOAD_PROGRAM_WORD(addr)     pgm_read_word_far(addr)
#endif /* LARGE_MEMORY */
#define _FILL_TEMP_WORD(addr,data)     boot_page_fill(addr, data)
#define _PAGE_ERASE(addr)              boot_page_erase(addr)
#define _PAGE_WRITE(addr)              boot_page_write(addr)

/* eeprom inteface */

#define _EEPROM_WRITE(addr,data)       eeprom_write_byte( (uint8_t *)addr, data )
#define _EEPROM_READ(addr)             eeprom_read_byte( (uint8_t *)addr )
#endif                                  /* _lint */

#endif                                  /* FLASH_H_ */

/* EOF */
