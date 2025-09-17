# Instrucciones de Trabajo para Codex

Este archivo define las reglas de interacción y generación de código que **Codex** debe seguir al continuar el desarrollo de este proyecto.

---

## Lineamientos Generales

1. **Tiempo de Respuesta**
   - Cada respuesta no debe tardar más de **10 minutos** en generarse.
   - Es preferible responder en partes pequeñas y funcionales en lugar de algo grande, incompleto o con errores.

2. **Implementación por Fases**
   - Cada vez que se inicie un chat con algo como:
     ```
     "Tú serás el encargado de la fase 'x' del plan de implementación para 'x' cosa"
     ```
     deberás:
     - Proponer un **plan de acción detallado** para esa fase y rol.
     - Preguntar explícitamente si debes empezar a implementar.
     - Una vez confirmado, **revisar el conocimiento del proyecto** (repositorio, README, etc.) antes de generar cualquier código.

3. **Modularidad y Gradualidad**
   - Nunca intentes crear o modificar todo en un solo prompt.
   - La implementación debe ser **archivo por archivo** y **parte por parte** dentro de cada archivo.
   - Evita modificar un archivo completo de golpe: los cambios deben hacerse en **bloques pequeños y explicados**.

4. **Proceso de Implementación**
   - Al entregar un bloque de código:
     - Detén la respuesta ahí.
     - Espera confirmación de que se colocó correctamente y compila antes de continuar.
   - Si se necesita modificar múltiples archivos, se puede hacer en pasos pequeños pero coordinados entre ellos.

5. **Optimización de Tokens**
   - No generes bloques demasiado grandes de código en una sola respuesta.
   - Respuestas largas con muchos errores consumen demasiados tokens en correcciones.
   - El objetivo es avanzar con pasos pequeños y **funcionales**, asegurando builds limpios.

6. **Restricciones de Estilo**
   - **No usar emojis** en código ni en archivos/documentos técnicos.
   - La explicación conversacional fuera del código sí puede incluirlos si se desea.

---

## Objetivo de Estas Reglas

- Reducir errores y regresiones.
- Mantener el desarrollo ordenado y progresivo.
- Evitar desperdicio de tokens en respuestas largas y fallidas.
- Garantizar que cada bloque entregado sea funcional y fácil de validar.

---