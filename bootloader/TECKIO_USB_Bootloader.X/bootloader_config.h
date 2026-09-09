#ifndef TECKIO_BOOTLOADER_CONFIG_H
#define TECKIO_BOOTLOADER_CONFIG_H

#include <stdint.h>

/* PIC18F45K50 program-memory map (byte addresses). */
#define TKBL_BOOT_START          0x0000UL
#define TKBL_BOOT_END            0x2000UL
#define TKBL_APP_START           0x2000UL
#define TKBL_METADATA_START      0x7FC0UL
#define TKBL_APP_END             TKBL_METADATA_START
#define TKBL_FLASH_END           0x8000UL

#define TKBL_FLASH_ROW_SIZE      64u
#define TKBL_MAX_DATA_CHUNK      64u

/* Reserved EEPROM byte used by the running application to request the loader. */
#define TKBL_EEPROM_REQUEST_ADDR 0xFFu
#define TKBL_EEPROM_REQUEST_KEY  0xA5u

#endif
