# PIC18F45K50 Basic Examples

Ejemplos y firmware oficial para la tarjeta de desarrollo TECKIO basada en el
**PIC18F45K50-I/PT TQFP-44**.

## Bootloader y aplicación TECKIO

- `bootloader/TECKIO_USB_Bootloader.X`: bootloader USB CDC.
- `application/Aplicacion_TECKIO.X`: aplicación base enlazada desde `0x2000`.
- `docs/PICkit_preservacion.md`: configuración para conservar `0x0000-0x1FFF`.
- `dist/TECKIO_factory.hex`: imagen completa de recuperación, generada y
  validada por la compilación automática.

La aplicación incluida enumera como puerto COM, responde a `1`, acepta
`SYS.INFO?`, `SYS.STATUS?`, `SYS.RESET` y `BOOT.ENTER`, y ejecuta la prueba de
GPIO cada 500 ms.

## Compilación

Requiere MPLAB XC8. Desde la raíz:

```sh
git submodule update --init --recursive
make all
```

Se generan:

- `dist/TECKIO_bootloader.hex`
- `dist/Aplicacion_TECKIO.hex`
- `dist/TECKIO_factory.hex`

El bootloader utiliza `0x0000-0x1FFF`; las aplicaciones se compilan con
`-mcodeoffset=0x2000` y reservan `0x7FC0-0x7FFF`.

La pila USB se referencia como submódulo desde `johnnydrazzi/USB-Stack`, fijada
a una revisión MIT. No se mantiene una copia duplicada dentro del proyecto.
