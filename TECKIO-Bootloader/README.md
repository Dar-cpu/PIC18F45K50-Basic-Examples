# TECKIO Bootloader

Esta carpeta está reservada para el **bootloader USB de la tarjeta TECKIO PIC18F45K50**.

No contiene ejemplos de usuario ni el proyecto fuente completo del bootloader.

## Uso previsto

Aquí se publicará posteriormente el archivo `.hex` final de recuperación, pensado para restaurar una tarjeta mediante **PICkit** si el bootloader fue sobrescrito o necesita reinstalarse.

## Memoria

- Bootloader: `0x0000-0x1FFF`
- Aplicación de usuario: desde `0x2000`

Las aplicaciones compiladas para programación por USB deben respetar esta distribución y no sobrescribir el área del bootloader.

## Entrada al bootloader

1. Presiona el botón físico **RESET**.
2. Se abre una ventana de aproximadamente **30 segundos**.
3. La app Windows o Android puede conectarse al bootloader.
4. Al comenzar la comunicación, la sesión permanece activa durante la programación.

## Archivo pendiente

El `.hex` de recuperación se añadirá aquí cuando se congele la versión final del firmware de fábrica.
