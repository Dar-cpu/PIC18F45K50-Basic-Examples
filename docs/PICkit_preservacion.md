# PICkit: conservar el bootloader TECKIO

| Archivo | Uso |
|---|---|
| Aplicacion_TECKIO.hex | USB: solo código 0x2000–0x7FBF; el bootloader crea su marca válida |
| Aplicacion_TECKIO_ICSP.hex | PICkit: código + nueva marca 0x7FC0–0x7FFF + configuración TECKIO |
| TECKIO_factory.hex | Recuperación: bootloader + aplicación + marca + configuración |

## Perfil de alumnos

En MPLAB IPE Advanced Mode / Memories configura y guarda un perfil local:

- Device PIC18F45K50; PICkit correspondiente; velocidad **Low**.
- Alimentación externa de la placa.
- Carga **Aplicacion_TECKIO_ICSP.hex** generado para ESA compilación.
- Program Memory activada; rango de aplicación **2000–7FFF**.
- Preserve Program Memory activado, rango **0000–1FFF**.
- Configuration Memory activada para restaurar la configuración TECKIO incluida.
- CPB/CP0 desactivados: la preservación requiere poder leer el bootloader.

Los nombres y disponibilidad dependen de IPE/PICkit. Comprueba que Preserve sigue
activo antes de Program Device. Es una receta, no un perfil universal importable;
falta comprobar una primera carga físicamente en tu placa.

Excluir Configuration Memory no garantiza conservar fuses ante borrado global.
Por eso el HEX ICSP incluye los fuses de fábrica (LVP=ON, ICPRT=ON, MCLRE=OFF,
WRTB/WRT0/WRTC=ON). No añadas pragma config a las fuentes del alumno.

La marca válida NO se preserva de la aplicación anterior: se genera para la nueva.
El HEX USB puro cargado por PICkit tras borrar la marca deja el bootloader activo.

## Recuperación completa

Desactiva Preserve, selecciona toda Program Memory y Configuration Memory,
carga **TECKIO_factory.hex**, ejecuta borrado completo, Program y Verify.
Esto sustituye el programa previo. Mantén Low y alimentación estable.

Preserve lee y restaura el área reservada. No garantiza supervivencia a cortes
de energía ni a Erase Device separado. WRT protege de escritura interna, no del
borrado externo. Conserva el factory HEX fuera de la placa.

Referencia Microchip:
https://onlinedocs.microchip.com/oxy/GUID-8D61C0B9-A97F-4F4D-99F8-1D7424264C2A-en-US-1/GUID-EEAC1745-D5FB-4EB3-AF63-D888F869D326.html
