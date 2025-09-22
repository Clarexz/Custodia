# Meshtastic Radio Profiles - Technical Specifications

Este documento detalla los parámetros técnicos de los perfiles de radio disponibles en Meshtastic, organizados desde el más rápido hasta el más lento.

## Patrones de Configuración

Los perfiles siguen un patrón lineal:
- **Velocidad**: Más rápido ↔ Más lento
- **Alcance**: Menor alcance ↔ Mayor alcance
- **Tiempo en aire**: Menor airtime ↔ Mayor airtime

## Perfiles de Radio Disponibles

### SHORT_TURBO
**Perfil más rápido - Menor alcance**

| Parámetro | Valor |
|-----------|-------|
| Bandwidth | 500 kHz |
| Spreading Factor | 7 |
| Coding Rate | 4/5 |
| TX Power | 20 dBm |
| Preamble Length | 8 |

**Características:**
- Velocidad máxima de transmisión
- Mayor ancho de banda disponible
- Menor tiempo en aire
- Alcance más corto
- ⚠️ **NO LEGAL en todas las regiones** debido al ancho de banda de 500kHz

---

### SHORT_FAST
**Alta velocidad - Alcance corto**

| Parámetro | Valor |
|-----------|-------|
| Bandwidth | 250 kHz |
| Spreading Factor | 7 |
| Coding Rate | 4/5 |
| TX Power | 20 dBm |
| Preamble Length | 8 |

**Características:**
- Velocidad alta
- Menor tiempo en aire que perfiles LONG
- Ideal para redes densas urbanas
- Alcance menor que LONG_FAST

---

### SHORT_SLOW
**Velocidad moderada-alta - Alcance corto-medio**

| Parámetro | Valor |
|-----------|-------|
| Bandwidth | 250 kHz |
| Spreading Factor | 8 |
| Coding Rate | 4/5 |
| TX Power | 20 dBm |
| Preamble Length | 8 |

**Características:**
- Balance entre SHORT_FAST y MEDIUM_FAST
- Mejor alcance que SHORT_FAST
- Velocidad menor que SHORT_FAST pero mayor que MEDIUM

---

### MEDIUM_FAST
**Balance óptimo velocidad-alcance**

| Parámetro | Valor |
|-----------|-------|
| Bandwidth | 250 kHz |
| Spreading Factor | 9 |
| Coding Rate | 4/5 |
| TX Power | 20 dBm |
| Preamble Length | 8 |

**Características:**
- Balance óptimo para redes medianas
- 3-4 veces más rápido que LONG_FAST
- Alcance respetable
- Recomendado para redes en crecimiento

---

### MEDIUM_SLOW
**Alcance moderado - Velocidad moderada**

| Parámetro | Valor |
|-----------|-------|
| Bandwidth | 250 kHz |
| Spreading Factor | 10 |
| Coding Rate | 4/5 |
| TX Power | 20 dBm |
| Preamble Length | 8 |

**Características:**
- Mayor alcance que MEDIUM_FAST
- Velocidad moderada
- Buena opción para áreas suburbanas
- Menos congestionado que LONG_FAST

---

### LONG_FAST
**Perfil estándar por defecto**

| Parámetro | Valor |
|-----------|-------|
| Bandwidth | 250 kHz |
| Spreading Factor | 11 |
| Coding Rate | 4/8 |
| TX Power | 20 dBm |
| Preamble Length | 8 |

**Características:**
- Perfil por defecto de Meshtastic
- Balance clásico entre alcance y velocidad
- ~1 kbps de velocidad de datos
- Aproximadamente 2.5km de alcance típico

---

### LONG_MODERATE
**Alcance extendido - Velocidad moderada-baja**

| Parámetro | Valor |
|-----------|-------|
| Bandwidth | 125 kHz |
| Spreading Factor | 11 |
| Coding Rate | 4/6 |
| TX Power | 20 dBm |
| Preamble Length | 8 |

**Características:**
- Mayor alcance que LONG_FAST
- Menor ancho de banda para mejor sensibilidad
- Velocidad moderada-baja
- Ideal para conexiones de larga distancia

---

### LONG_SLOW
**Máximo alcance - Menor velocidad**

| Parámetro | Valor |
|-----------|-------|
| Bandwidth | 125 kHz |
| Spreading Factor | 12 |
| Coding Rate | 4/8 |
| TX Power | 20 dBm |
| Preamble Length | 8 |

**Características:**
- Máximo alcance disponible
- Menor velocidad de transmisión
- Mayor tiempo en aire
- Ideal para enlaces de emergencia de larga distancia

---

## Consideraciones de Implementación

### Legalidad Regional
- **SHORT_TURBO**: No legal en Europa debido al ancho de banda de 500kHz
- **Otros perfiles**: Generalmente legales en la mayoría de regiones

### Compatibilidad de Red
- Todos los nodos en una red deben usar el mismo perfil
- No es posible mezclar perfiles diferentes en la misma red
- La migración requiere actualización coordinada de todos los nodos

### Recomendaciones de Uso

| Escenario | Perfil Recomendado | Razón |
|-----------|-------------------|-------|
| Red urbana densa (20+ nodos) | SHORT_FAST, MEDIUM_FAST | Reduce congestión del canal |
| Red suburbana (10-20 nodos) | MEDIUM_SLOW, LONG_FAST | Balance velocidad/alcance |
| Red rural dispersa | LONG_MODERATE, LONG_SLOW | Maximiza alcance |
| Pruebas de máximo alcance | LONG_SLOW | Máxima sensibilidad |
| Red experimental rápida | SHORT_TURBO* | Máxima velocidad (*donde sea legal) |

### Factores de Rendimiento

| Factor | SHORT profiles | MEDIUM profiles | LONG profiles |
|--------|---------------|----------------|---------------|
| Velocidad datos | Alta | Moderada | Baja |
| Tiempo en aire | Bajo | Moderado | Alto |
| Probabilidad colisión | Baja | Moderada | Alta |
| Alcance típico | 0.5-1.5km | 1-2.5km | 2-5km+ |
| Escalabilidad red | Excelente | Buena | Limitada |

## Parámetros Técnicos Explicados

### Spreading Factor (SF)
- **Rango**: 7-12
- **7**: Más rápido, menor alcance
- **12**: Más lento, mayor alcance
- Cada incremento dobla el tiempo en aire

### Bandwidth (BW)
- **125 kHz**: Mayor sensibilidad, menor velocidad
- **250 kHz**: Balance estándar
- **500 kHz**: Mayor velocidad, menor sensibilidad

### Coding Rate (CR)
- **4/5**: Menos redundancia, más rápido
- **4/6**: Redundancia moderada
- **4/8**: Máxima redundancia, más robusto

### TX Power
- **Rango**: 2-22 dBm (región dependiente)
- **20 dBm**: Valor por defecto para la mayoría de regiones
- **Ajustar según regulaciones locales**

---

## Referencias
- [Documentación oficial Meshtastic](https://meshtastic.org/docs/configuration/radio/lora/)
- [Configuración de canales](https://meshtastic.org/docs/configuration/radio/channels/)
- [Pruebas de alcance](https://meshtastic.org/docs/overview/range-tests/)