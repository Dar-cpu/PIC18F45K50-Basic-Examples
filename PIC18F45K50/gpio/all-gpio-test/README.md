# GPIO — prueba general

Prueba básica para verificar los GPIO disponibles de la tarjeta TECKIO PIC18F45K50.

El programa conmuta simultáneamente los GPIO seguros cada **500 ms**.

Se mantienen reservados:

- `RA6` y `RA7`: cristal de la tarjeta.
- `RC4` y `RC5`: USB D− / D+.
- `RE3`: entrada.

Copia `main.c` dentro de tu proyecto MPLAB X. Si programas por USB mediante el bootloader TECKIO, utiliza el proyecto/template configurado para iniciar la aplicación en `0x2000`.
