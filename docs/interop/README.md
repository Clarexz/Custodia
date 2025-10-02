Interoperabilidad Meshtastic — Fase 0
====================================

Objetivo de la fase
- Integrar los `.proto` oficiales de Meshtastic y preparar tooling de build, sin tocar el resto del firmware.

Versión objetivo
- Última estable compatible con Solar Node P1 Pro.
- Bloqueo (commit) tomado del submódulo `protobufs` referenciado por `meshtastic/firmware@master` y guardado en `third_party/meshtastic_protos/protos.lock`.

Región y preset de validación
- Región: US915
- Perfil: Urban_DENSE (ajuste en las fases de radio: Fase 3)

Estructura agregada en Fase 0
- `scripts/interop/regen_protos.py`: descarga `.proto` oficiales a `third_party/meshtastic_protos/`.
- `extra_scripts/meshtastic_codegen.py`: extra script (opcional) de PlatformIO para futura generación con nanopb.
- `third_party/meshtastic_protos/`: carpeta para los `.proto` y `protos.lock`.

Uso rápido
1) Descargar `.proto`:
   - `python3 scripts/interop/regen_protos.py`
2) (Opcional) Verificar en el build log que el extra script reconoce el material:
   - En `platformio.ini`, puedes habilitar temporalmente:
     - `extra_scripts = extra_scripts/meshtastic_codegen.py` (comentado por defecto)
   - Exporta `MESHTASTIC_INTEROP_GEN=1` si quieres intentar codegen en fases siguientes.

Notas
- Fase 0 no genera código C/C++; solo integra definiciones y tooling.
- La generación de nanopb y su integración en el build se hará en Fase 1.

