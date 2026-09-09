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
- `dist/Aplicacion_TECKIO_ICSP.hex` (aplicación, marca válida y configuración; sin bootloader)
- `dist/TECKIO_factory.hex`

El bootloader utiliza `0x0000-0x1FFF`; las aplicaciones se compilan con
`-mcodeoffset=0x2000` y reservan `0x7FC0-0x7FFF`.

Usa el HEX puro por USB y el HEX `_ICSP` con PICkit y preservación habilitada.
La imagen `TECKIO_factory.hex` es exclusivamente para recuperación completa por ICSP.
No envíes `_ICSP` ni `factory` al cargador USB.

Las carpetas `.X` contienen fuentes; la compilación reproducible usa el Makefile raíz.
Para crear el proyecto gestionado por MPLAB X, sigue `docs/Aplicacion_MPLAB.md`.
Los HEX compilados se descargan en Actions → último build satisfactorio → Artifacts;
no están versionados en Git. Compilación comprobada no equivale a prueba física en placa.

La pila USB se referencia como submódulo desde `johnnydrazzi/USB-Stack`, fijada
a una revisión MIT. No se mantiene una copia duplicada dentro del proyecto.
