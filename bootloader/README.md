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
