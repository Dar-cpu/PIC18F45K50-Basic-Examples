# PIC18F45K50 Basic Examples

Colección de ejemplos prácticos en C para la tarjeta de desarrollo **TECKIO PIC18F45K50**, usando **MPLAB X IDE** y **XC8**.

Este repositorio contiene únicamente ejemplos de usuario. El bootloader, las aplicaciones de producción, herramientas de generación y archivos de compilación no forman parte de este repositorio.

## Tarjeta de desarrollo

<p align="center">
  <img src="Images/teckio_pic18f45k50_dev.jpeg" alt="Tarjeta de desarrollo TECKIO PIC18F45K50" width="680">
</p>

## Organización

| Carpeta | Contenido |
| --- | --- |
| [`PIC18F45K50/`](PIC18F45K50) | Ejemplos para el PIC18F45K50 |
| [`Images/`](Images) | Imagen general de la tarjeta |

## Ejemplos disponibles

| Categoría | Ejemplo |
| --- | --- |
| GPIO | [Prueba general de GPIO](PIC18F45K50/gpio/all-gpio-test) |

Se añadirán nuevos ejemplos de ADC, timers, PWM, UART, I²C, SPI y USB conforme sean compilados y verificados en hardware.

## Cómo usar los ejemplos

1. Crea un **Standalone Project** en MPLAB X.
2. Selecciona **PIC18F45K50** y el compilador **XC8**.
3. Si vas a programar mediante el bootloader TECKIO, usa el template/configuración de proyecto que inicia la aplicación en `0x2000` para no sobrescribir el bootloader.
4. Copia el `main.c` del ejemplo deseado.
5. Compila y carga el `.hex` generado.

## Estructura

```text
PIC18F45K50-Basic-Examples/
├── Images/
├── PIC18F45K50/
│   └── gpio/
│       └── all-gpio-test/
│           ├── README.md
│           └── main.c
├── LICENSE
└── README.md
```

## Licencia

Código distribuido bajo la [licencia MIT](LICENSE).
