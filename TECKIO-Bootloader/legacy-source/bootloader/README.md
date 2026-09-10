# Bootloader USB TECKIO

Bootloader CDC para PIC18F45K50-I/PT TQFP-44.

Mapa de memoria:

- `0x0000-0x1FFF`: bootloader protegido.
- `0x2000-0x7FBF`: aplicación.
- `0x7FC0-0x7FFF`: metadatos de validez.

Usa el puerto ICSP dedicado de los pines 12, 13 y 33 (`ICPRT=ON`) y conserva
LVP (`LVP=ON`). `CP0` y `CPB` permanecen desactivados para permitir la función
de preservación de MPLAB. `WRT0`, `WRTB` y `WRTC` protegen frente a escrituras
internas accidentales.

La compilación usa la pila CDC incluida como submódulo en
`third_party/USB-Stack`.

## Entrada al bootloader

Con una aplicación válida, el encendido y los reinicios por software arrancan
directamente desde `0x2000`. Para cargar otro HEX no hace falta implementar ni
enviar `BOOT.ENTER`:

1. Pulsa el botón físico **RESET** de la tarjeta.
2. Durante 30 segundos, conecta la app Windows o Android y pulsa programar.
3. El primer `HELLO` mantiene el bootloader activo hasta terminar.
4. Después de `VERIFY` y `RESET`, la aplicación nueva arranca inmediatamente.

Si no existe una aplicación válida, el bootloader permanece activo sin límite.
`MCLRE=ON` mantiene operativo el botón RESET.
