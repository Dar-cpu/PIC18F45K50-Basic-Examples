#include <xc.h>
#include <stdint.h>

#define TKBL_EEPROM_REQUEST_ADDR 0xFFu
#define TKBL_EEPROM_REQUEST_KEY  0xA5u

#include "teckio_boot_api.h"

void teckio_enter_bootloader(void)
{
    uint8_t saved_gie;

    EEADR = TKBL_EEPROM_REQUEST_ADDR;
    EEDATA = TKBL_EEPROM_REQUEST_KEY;
    EECON1bits.EEPGD = 0;
    EECON1bits.CFGS = 0;
    EECON1bits.WREN = 1;

    saved_gie = INTCONbits.GIE;
    INTCONbits.GIE = 0;
    EECON2 = 0x55;
    EECON2 = 0xAA;
    EECON1bits.WR = 1;
    while (EECON1bits.WR) {}
    EECON1bits.WREN = 0;
    INTCONbits.GIE = saved_gie;

    RESET();
}
