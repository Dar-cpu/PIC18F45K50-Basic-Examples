#ifndef TECKIO_CONFIG_H
#define TECKIO_CONFIG_H

/*
 * Minimal configuration expected by the USB stack.
 * This test intentionally uses the internal 16 MHz oscillator
 * with PLL x3 for USB Full-Speed.
 */

#define NO_XTAL 0
#define MHz_12  1
#define MHz_16  2

#define XTAL_USED NO_XTAL

#define PLL_STARTUP_DELAY() __delay_ms(3)

#endif
