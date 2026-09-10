# PIC18F45K50 Basic Examples

Colección de ejemplos prácticos en C para la tarjeta de desarrollo **TECKIO PIC18F45K50**, usando **MPLAB X IDE** y **XC8**.

Los ejemplos de usuario se mantienen separados del firmware interno de la tarjeta. El bootloader USB y sus fuentes de recuperación están en una carpeta independiente para no mezclarlo con los ejemplos.

## Tarjeta de desarrollo

<p align="center">
  <img src="Images/teckio_pic18f45k50_dev.jpeg" alt="Tarjeta de desarrollo TECKIO PIC18F45K50" width="680">
</p>

## Organización

| Carpeta | Contenido |
| --- | --- |
| [`PIC18F45K50/`](PIC18F45K50) | Ejemplos para el PIC18F45K50 |
| [`TECKIO-Bootloader/`](TECKIO-Bootloader) | Bootloader USB, fuentes anteriores y recuperación |
| [`Images/`](Images) | Imagen general de la tarjeta |

## Ejemplos disponibles

| Categoría | Ejemplo |
| --- | --- |
| GPIO | [Prueba general de GPIO](PIC18F45K50/gpio/all-gpio-test) |

Se añadirán nuevos ejemplos de ADC, timers, PWM, UART, I²C, SPI y USB conforme sean compilados y verificados en hardware.

## Cómo usar los ejemplos

1. Crea un **Standalone Project** en MPLAB X.
2. Selecciona **PIC18F45K50** y **XC8**.
3. Si programas mediante el bootloader TECKIO, configura la aplicación para iniciar desde `0x2000` y no sobrescribir `0x0000-0x1FFF`.
4. Copia el `main.c` del ejemplo deseado.
5. Compila y carga el `.hex` generado.

## Estructura

```text
PIC18F45K50-Basic-Examples/
├── Images/
├── PIC18F45K50/
│   └── gpio/
│       └── all-gpio-test/
├── TECKIO-Bootloader/
│   ├── legacy-source/
│   └── recovery/
├── LICENSE
└── README.md
```

## Licencia

Código distribuido bajo la [licencia MIT](LICENSE). Las dependencias de terceros conservan sus avisos y licencias correspondientes.
