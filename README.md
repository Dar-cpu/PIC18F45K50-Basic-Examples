# PIC18F45K50 Basic Examples

Colección de ejemplos prácticos en **C** para el microcontrolador **Microchip PIC18F45K50-I/PT** y la tarjeta de desarrollo **TECKIO PIC18F45K50**, usando **MPLAB X IDE** y **XC8**.

El repositorio está enfocado en ejemplos simples, independientes y fáciles de reutilizar. El bootloader de la tarjeta se mantiene separado de los ejemplos y su carpeta queda reservada para publicar posteriormente el `.hex` de recuperación.

## Organización principal

| Carpeta | Contenido |
| --- | --- |
| [`PIC18F45K50/`](PIC18F45K50) | Ejemplos básicos organizados por periférico |
| [`TECKIO-Bootloader/`](TECKIO-Bootloader) | Carpeta reservada para el `.hex` final del bootloader/firmware de recuperación |
| [`Images/`](Images) | Fotografías generales de la tarjeta |

## Ejemplos disponibles

| Categoría | Ejemplo |
| --- | --- |
| GPIO | [Prueba general de GPIO](PIC18F45K50/gpio/all-gpio-test) |

Se añadirán ejemplos de **ADC, Timer, PWM, UART, I²C, SPI y USB** conforme sean compilados y verificados en hardware.

## Cómo usar los ejemplos

1. Crea un **Standalone Project** en MPLAB X.
2. Selecciona **PIC18F45K50**.
3. Selecciona el compilador **XC8**.
4. Copia el `main.c` del ejemplo deseado.
5. Compila el proyecto y programa la tarjeta.

### Si utilizas el bootloader TECKIO

La aplicación debe comenzar desde `0x2000` para no sobrescribir el bootloader ubicado en `0x0000-0x1FFF`.

Puedes usar el template/configuración de MPLAB X preparado para aplicaciones TECKIO antes de generar el `.hex` que cargarás mediante USB.

## Bootloader

La carpeta [`TECKIO-Bootloader/`](TECKIO-Bootloader) **no contiene ejemplos de usuario**.

Se mantiene únicamente para publicar posteriormente el `.hex` final necesario para recuperar una tarjeta mediante PICkit en caso de que el bootloader sea sobrescrito o necesite restaurarse.

## Estructura resumida

```text
PIC18F45K50-Basic-Examples/
├── Images/
├── PIC18F45K50/
│   └── gpio/
│       └── all-gpio-test/
├── TECKIO-Bootloader/
│   └── README.md
├── LICENSE
└── README.md
```

## Estado del repositorio

Los ejemplos se incorporan después de ser compilados y verificados físicamente en la tarjeta TECKIO PIC18F45K50.

## Licencia

El código se distribuye bajo la [licencia MIT](LICENSE).
