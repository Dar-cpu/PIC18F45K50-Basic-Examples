# TECKIO Bootloader

Esta carpeta conserva el firmware del **bootloader USB de la TECKIO PIC18F45K50** que quedó funcionando en el commit `24f2eacf16454e2dcc80ed29f76413e33ffcaae4`.

No es un ejemplo de usuario. Está separado para recuperación, mantenimiento y referencia del firmware de fábrica.

## Organización

```text
TECKIO-Bootloader/
├── legacy-source/   # copia completa del estado funcional anterior
└── recovery/        # lugar para el HEX final de recuperación
```

Dentro de `legacy-source/` se conservan tal como estaban:

- `bootloader/TECKIO_USB_Bootloader.X/`
- `application/Aplicacion_TECKIO.X/`
- `usb/` con los descriptores TECKIO
- `third_party/USB-Stack/` fijado a la revisión usada por el firmware
- `docs/`
- `tools/`
- `Makefile`
- workflow y archivos auxiliares del build original

## Pila USB

El firmware funcional utilizó `johnnydrazzi/USB-Stack` en la revisión:

`c22203a6f52e0500ed47f795da85194c662a6118`

Ahí se encuentran `usb.c`, `usb.h`, `usb_cdc_acm.c`, `usb_cdc.h`, `usb_app.c` y los demás archivos USB usados por el build.

Para descargar también el contenido del submódulo:

```bash
git clone --recurse-submodules https://github.com/Dar-cpu/PIC18F45K50-Basic-Examples.git
```

## Memoria

- Bootloader: `0x0000-0x1FFF`.
- Aplicación: desde `0x2000`.
- La aplicación debe preservar el área del bootloader.

## Entrada al bootloader

1. Presiona el botón físico **RESET**.
2. Se abre una ventana de aproximadamente **30 segundos**.
3. La app Windows o Android se conecta al bootloader.
4. El primer `HELLO` mantiene activa la sesión hasta terminar la programación.

## Recuperación

La carpeta [`recovery/`](recovery) queda reservada para el `.hex` de recuperación. Cuando el firmware quede congelado para usuarios finales, se podrá dejar ahí únicamente el HEX necesario para recuperar una tarjeta con PICkit y reducir las fuentes si se desea.
