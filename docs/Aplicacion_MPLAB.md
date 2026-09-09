# Aplicación TECKIO desde 0x2000

La fuente ya está preparada y el Makefile aplica offset y reserva. GitHub compila
con XC8 3.10 y PIC18F-K DFP 1.17.312. Las carpetas `.X` contienen fuentes, no los
metadatos dependientes de tu instalación MPLAB/PICkit.

Para crear un proyecto gestionado por MPLAB X:

1. Clona con submódulos o ejecuta `git submodule update --init --recursive`.
2. Crea un Standalone Project, PIC18F45K50, XC8.
3. Añade main.c y teckio_boot_api.c de application/Aplicacion_TECKIO.X.
4. Añade usb.c y usb_cdc_acm.c de third_party/USB-Stack/USB_Stack/USB;
   usb_app.c de Examples/CDC_Examples/Shared_Files y usb/teckio_descriptors.c
   de este repositorio. No agregues por separado usb_descriptors.c del submódulo.
5. Include directories: application/Aplicacion_TECKIO.X, USB y Shared_Files.
6. XC8 linker: Code Offset 0x2000 y ROM reservada 7FC0–7FFF. Comprueba los argumentos:
   `-mcodeoffset=0x2000 -mreserve=rom@7fc0:7fff`.
7. Compila Release. No añadas fuentes del bootloader ni pragma config.

Reset: 0x2000. Vectores: 0x2008/0x2018, reenviados por el bootloader.
Para empaquetar una aplicación nueva para PICkit:

```sh
python tools/make_factory_hex.py --bootloader dist/TECKIO_bootloader.hex --application MI_APP.hex --output TECKIO_factory.hex --icsp-output Aplicacion_TECKIO_ICSP.hex
```

El generador valida checksums, rangos y ausencia de solapamientos. Rechaza fuses en
MI_APP.hex. Usa el HEX puro por USB. La prueba conmuta GPIO: desconecta las cargas
externas durante la prueba; USB, pines del cristal y el ICSP dedicado se reservan.
