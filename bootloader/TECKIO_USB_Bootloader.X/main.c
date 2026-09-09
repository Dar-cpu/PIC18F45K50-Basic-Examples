/*
 * TECKIO PIC18F45K50 USB CDC bootloader
 * Target: PIC18F45K50-I/PT, 44-pin TQFP, MPLAB XC8
 */

#include <xc.h>
#include <stdbool.h>
#include <stdint.h>

#define _XTAL_FREQ 48000000UL

#include "config.h"
#include "pic18_flash.h"
#include "teckio_protocol.h"
#include "usb.h"
#include "usb_cdc.h"

/* Known-working USB clock setup from the user's COM14 CDC test. */
#pragma config PLLSEL   = PLL3X
#pragma config CFGPLLEN = OFF
#pragma config CPUDIV   = NOCLKDIV
#pragma config LS48MHZ  = SYS48X8
#pragma config FOSC     = INTOSCIO
#pragma config PCLKEN   = OFF
#pragma config FCMEN    = OFF
#pragma config IESO     = OFF
#pragma config nPWRTEN  = OFF
#pragma config BOREN    = SBORDIS
#pragma config nLPBOR   = ON
#pragma config WDTEN    = OFF
#pragma config WDTPS    = 32768

/* RE3 is a digital input. Programming/debug stays on pins 12, 13 and 33. */
#pragma config MCLRE    = OFF
#pragma config PBADEN   = OFF
#pragma config CCP2MX   = RC1
#pragma config T3CMX    = RC0
#pragma config SDOMX    = RC7
#pragma config STVREN   = ON
#pragma config LVP      = ON
#pragma config ICPRT    = ON
#pragma config XINST    = OFF

/*
 * Accidental self-write protection.  The USB loader writes only Blocks 1-3.
 * Code protection stays disabled so MPLAB can preserve and restore the loader.
 */
#pragma config CP0      = OFF
#pragma config CP1      = OFF
#pragma config CP2      = OFF
#pragma config CP3      = OFF
#pragma config CPB      = OFF
#pragma config CPD      = OFF
#pragma config WRT0     = ON
#pragma config WRT1     = OFF
#pragma config WRT2     = OFF
#pragma config WRT3     = OFF
#pragma config WRTC     = ON
#pragma config WRTB     = ON
#pragma config WRTD     = OFF
#pragma config EBTR0    = OFF
#pragma config EBTR1    = OFF
#pragma config EBTR2    = OFF
#pragma config EBTR3    = OFF
#pragma config EBTRB    = OFF

static volatile bool usb_tx_done = true;
static volatile bool usb_rx_ready = false;
static volatile bool reset_pending = false;

static void clock_init(void);

/* Forward the fixed PIC18 interrupt vectors to the offset application. */
void __at(0x0008) __attribute__((used)) teckio_high_vector(void)
{
    __asm("goto 0x2008");
}

void __at(0x0018) __attribute__((used)) teckio_low_vector(void)
{
    __asm("goto 0x2018");
}

void main(void)
{
    uint8_t packet_length;

    clock_init();

    /* Normal boot is immediate; the application can request the loader in EEPROM. */
    if (!pic18_boot_request_take() && pic18_application_is_valid()) {
        pic18_jump_to_application();
    }

    ANSELA = 0x00;
    ANSELB = 0x00;
    ANSELC = 0x00;
    ANSELD = 0x00;
    ANSELE = 0x00;
    /* RE3 is input-only when MCLRE=OFF; it needs no TRIS bit write. */

    tkbl_init();
    usb_init();

    /* Polling leaves both hardware interrupt vectors available to the application. */
    INTCONbits.GIE = 0;
    INTCONbits.PEIE = 0;
    USB_INTERRUPT_ENABLE = 0;

    while (1) {
        usb_tasks();

        if (usb_get_state() < STATE_CONFIGURED) {
            continue;
        }

        if (usb_rx_ready) {
            packet_length = g_cdc_num_data_out;
            usb_rx_ready = false;
            tkbl_feed(g_cdc_dat_ep_out, packet_length);
            cdc_arm_data_ep_out();
        }

        if (reset_pending && usb_tx_done) {
            __delay_ms(20);
            RESET();
        }
    }
}

static void clock_init(void)
{
    /* Internal 16 MHz HFINTOSC, PLL x3 = 48 MHz USB full-speed clock. */
    OSCTUNE = 0x80;
    OSCCON = 0x70;
    OSCCON2 = 0x10;
    while (OSCCON2bits.PLLRDY != 1) {}

    /* USB Start-of-Frame is the clock tuning reference. */
    ACTCON = 0x90;
}

void cdc_set_control_line_state(void) {}
void cdc_set_line_coding(void) {}

void cdc_data_out(void)
{
    /* OUT is re-armed only after main() consumes this packet. */
    usb_rx_ready = true;
}

void cdc_data_in(void)
{
    usb_tx_done = true;
}

void cdc_notification(void) {}

void tkbl_platform_send(const uint8_t *data, uint8_t length)
{
    uint8_t i;

    while (!usb_tx_done) {
        usb_tasks();
    }
    for (i = 0; i < length; ++i) {
        g_cdc_dat_ep_in[i] = data[i];
    }
    usb_tx_done = false;
    cdc_arm_data_ep_in(length);
}

void tkbl_platform_reset(void)
{
    reset_pending = true;
}
