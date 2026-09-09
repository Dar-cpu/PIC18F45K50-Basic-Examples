/*
 * Aplicacion TECKIO para PIC18F45K50-I/PT.
 *
 * Este proyecto se enlaza desde 0x2000. No contiene bits de configuracion:
 * pertenecen exclusivamente al bootloader de fabrica.
 *
 * Prueba incluida:
 *   - USB CDC a 115200 8N1 (velocidad nominal).
 *   - Responde "TECKIO USB OK" al recibir "1".
 *   - Comandos SYS.INFO?, SYS.STATUS?, SYS.RESET y BOOT.ENTER.
 *   - Conmuta simultaneamente todos los GPIO seguros cada 500 ms.
 */

#include <xc.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define _XTAL_FREQ 48000000UL

#include "config.h"
#include "teckio_boot_api.h"
#include "usb.h"
#include "usb_cdc.h"

static volatile bool usb_tx_done = true;
static volatile bool usb_rx_ready = false;
static char command[64];
static uint8_t command_length;
static bool command_overflow;
static uint8_t blink_ticks = 0;
static bool pins_on = false;

static void clock_init(void);
static void gpio_init(void);
static void gpio_test_write(bool on);
static void process_rx(void);
static void usb_send_text(const char *text);
static bool usb_send_bytes(uint8_t length);
static void execute_command(void);

static void __interrupt() isr(void)
{
    if (USB_INTERRUPT_ENABLE && USB_INTERRUPT_FLAG) {
        usb_tasks();
        USB_INTERRUPT_FLAG = 0;
    }
}

void main(void)
{
    clock_init();
    gpio_init();
    usb_init();

    INTCONbits.PEIE = 1;
    USB_INTERRUPT_FLAG = 0;
    USB_INTERRUPT_ENABLE = 1;
    INTCONbits.GIE = 1;

    while (1) {
        if (usb_get_state() < STATE_CONFIGURED) {
            command_length = 0;
            command_overflow = false;
            usb_tx_done = true;
        } else {
            if (usb_rx_ready) {
                process_rx();
            }
        }

        __delay_ms(10);
        if (++blink_ticks >= 50u) {
            blink_ticks = 0;
            pins_on = !pins_on;
            gpio_test_write(pins_on);
        }
    }
}

static void clock_init(void)
{
    /* Misma base de reloj USB ya validada: 16 MHz internos x PLL3 = 48 MHz. */
    OSCTUNE = 0x80;
    OSCCON = 0x70;
    OSCCON2 = 0x10;
    while (OSCCON2bits.PLLRDY != 1) {}
    ACTCON = 0x90;
}

static void gpio_init(void)
{
    ADCON0bits.ADON = 0;
    CM1CON0bits.C1ON = 0;
    CM2CON0bits.C2ON = 0;

    ANSELA = 0x00;
    ANSELB = 0x00;
    ANSELC = 0x00;
    ANSELD = 0x00;
    ANSELE = 0x00;

    LATA = 0x00;
    LATB = 0x00;
    LATC = 0x00;
    LATD = 0x00;
    LATE = 0x00;

    /* RA6/RA7 tienen el cristal soldado y no se fuerzan como salidas. */
    TRISA = 0xC0;
    TRISB = 0x00;
    /* RC4/RC5 pertenecen a USB D-/D+. */
    TRISC = 0x30;
    TRISD = 0x00;
    /* RE3 permanece como entrada digital; RE0-RE2 son salidas. */
    TRISE = 0x08;
}

static void gpio_test_write(bool on)
{
    if (on) {
        LATA = 0x3F;
        LATB = 0xFF;
        LATC = 0xC7;
        LATD = 0xFF;
        LATE = 0x07;
    } else {
        LATA = 0x00;
        LATB = 0x00;
        LATC = 0x00;
        LATD = 0x00;
        LATE = 0x00;
    }
}

void cdc_set_control_line_state(void) {}
void cdc_set_line_coding(void) {}

void cdc_data_out(void)
{
    usb_rx_ready = true;
}

void cdc_data_in(void)
{
    usb_tx_done = true;
}

void cdc_notification(void) {}

static void process_rx(void)
{
    uint8_t count = g_cdc_num_data_out;
    uint8_t i;
    usb_rx_ready = false;
    for (i = 0; i < count; ++i) {
        char ch = (char)g_cdc_dat_ep_out[i];
        if (ch == '\r' || ch == '\n') {
            if (command_overflow) usb_send_text("ERR LENGTH comando_largo\r\n");
            else if (command_length) {
                command[command_length] = '\0';
                execute_command();
            }
            command_length = 0;
            command_overflow = false;
        } else if (!command_overflow) {
            if (command_length < sizeof(command) - 1u) command[command_length++] = ch;
            else command_overflow = true;
        }
    }
    /* Preserve the original one-byte USB test without a line terminator. */
    if (command_length == 1u && command[0] == '1' && !command_overflow) {
        command[1] = '\0';
        execute_command();
        command_length = 0;
    }
    cdc_arm_data_ep_out();
}

static void execute_command(void)
{
    if (strcmp(command, "1") == 0) {
        usb_send_text("TECKIO USB OK\r\n");
    } else if (strcmp(command, "SYS.INFO?") == 0) {
        usb_send_text("OK board=TECKIO-PIC18F45K50 app=factory-test version=1.0.0\r\n");
    } else if (strcmp(command, "SYS.STATUS?") == 0) {
        usb_send_text("OK usb=ready test=gpio-blink\r\n");
    } else if (strcmp(command, "SYS.RESET") == 0) {
        usb_send_text("OK reset\r\n");
        __delay_ms(20);
        RESET();
    } else if (strcmp(command, "BOOT.ENTER") == 0) {
        usb_send_text("OK bootloader\r\n");
        __delay_ms(20);
        teckio_enter_bootloader();
    } else {
        usb_send_text("ERR UNKNOWN comando_no_reconocido\r\n");
    }

}

static void usb_send_text(const char *text)
{
    uint8_t length = 0;
    if (!usb_tx_done || usb_get_state() < STATE_CONFIGURED) return;

    while (*text != '\0') {
        g_cdc_dat_ep_in[length++] = (uint8_t)*text++;
        if (length == CDC_DAT_EP_SIZE) {
            if (!usb_send_bytes(length)) return;
            length = 0;
        }
    }
    if (length > 0u) {
        usb_send_bytes(length);
    }
}

static bool usb_send_bytes(uint8_t length)
{
    uint8_t timeout = 250u;
    usb_tx_done = false;
    cdc_arm_data_ep_in(length);
    while (!usb_tx_done && usb_get_state() >= STATE_CONFIGURED && timeout--) __delay_ms(1);
    return usb_tx_done;
}
