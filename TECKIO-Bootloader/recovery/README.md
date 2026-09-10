# Recuperación

Esta carpeta está reservada para el firmware `.hex` de recuperación de la tarjeta **TECKIO PIC18F45K50**.

El bootloader funcional conservado en `source/bootloader/` ocupa `0x0000-0x1FFF`. Cuando se publique una imagen de recuperación estable, debe colocarse aquí con un nombre claro, por ejemplo:

`TECKIO_PIC18F45K50_bootloader_recovery.hex`

El objetivo es que un usuario pueda recuperar una tarjeta mediante PICkit sin tener que revisar las fuentes internas del bootloader.
