# Aplicación TECKIO

Proyecto base para el PIC18F45K50 con bootloader TECKIO instalado.

- Reset e interrupciones reubicados con `-mcodeoffset=0x2000`.
- Región `0x7FC0-0x7FFF` reservada para la marca de firmware válido.
- Sin `#pragma config`; los bits de configuración pertenecen al bootloader.
- USB CDC operativo y comando `BOOT.ENTER` incluido.
- Prueba de fábrica: conmuta los GPIO seguros cada 500 ms.

El HEX resultante `Aplicacion_TECKIO.hex` puede cargarse desde TECKIO Control
Hub o TECKIO Dev Studio. Para usar PICkit, aplique la configuración descrita en
`docs/PICkit_preservacion.md`.

La compilación usa la pila CDC incluida como submódulo en
`third_party/USB-Stack`.
