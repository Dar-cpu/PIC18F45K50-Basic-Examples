/*
 * TECKIO PIC18F45K50 - prueba general de GPIO
 *
 * Conmuta simultaneamente todos los GPIO seguros cada 500 ms.
 * Este archivo no incluye Configuration Bits porque, al usar el bootloader
 * TECKIO, estos pertenecen al firmware de fabrica.
 */

#include <xc.h>
#include <stdbool.h>

#define _XTAL_FREQ 48000000UL

static void clock_init(void);
static void gpio_init(void);
static void gpio_write(bool on);

void main(void)
{
    bool state = false;

    clock_init();
    gpio_init();

    while (1) {
        state = !state;
        gpio_write(state);
        __delay_ms(500);
    }
}

static void clock_init(void)
{
    /* 16 MHz interno + PLL = 48 MHz. */
    OSCTUNE = 0x80;
    OSCCON = 0x70;
    OSCCON2 = 0x10;

    while (OSCCON2bits.PLLRDY != 1) {
    }

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

    /* RA6/RA7: cristal. */
    TRISA = 0xC0;
    TRISB = 0x00;

    /* RC4/RC5: USB D-/D+. */
    TRISC = 0x30;
    TRISD = 0x00;

    /* RE3 permanece como entrada. */
    TRISE = 0x08;
}

static void gpio_write(bool on)
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
