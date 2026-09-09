# PICkit: conservar el bootloader TECKIO

Esta configuración se usa solamente para grabar una **aplicación** con PICkit.
El HEX de aplicación debe comenzar en `0x2000` y no debe contener bits de
configuración.

## MPLAB X / IPE

En **Tool Properties > Memories to Program**:

- `Program Memory`: activado.
- `Program Memory Range`: `2000-7FBF`.
- `Preserve Program Memory`: activado.
- `Preserve Program Memory Range(s)`: `0-1FFF`.
- `Configuration Memory`: excluida para conservar la configuración de fábrica.

En **Program Options**:

- `Erase All Before Program`: activado.
- `Programming Speed`: `Low`.
- La tarjeta debe estar alimentada externamente y el PICkit debe detectar VDD.

Con estas opciones, **Program Device** conserva el bootloader. `Erase Device`
continúa siendo un borrado completo intencional; después se recupera la tarjeta
grabando `dist/TECKIO_factory.hex`.

No active `CP0` ni `CPB`: MPLAB necesita leer `0x0000-0x1FFF` para preservarlo.
La imagen de fábrica sí activa `WRT0`, `WRTB` y `WRTC`, que bloquean escrituras
accidentales realizadas por firmware, pero no impiden un borrado externo
intencional con PICkit.
