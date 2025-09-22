/*
 * GPS_LOGIC.H - Punto único de integración para datos GPS
 * 
 * Mantén los símbolos declarados aquí: puedes reemplazar la lógica interna en
 * gps_logic.cpp (simulación, lectura real, etc.) siempre que sigas
 * actualizando estas variables/funciones.
 */

#ifndef GPS_LOGIC_H
#define GPS_LOGIC_H

#include "gps_types.h"

extern GPSData gpsData;
extern GPSStatus gpsStatus;

void gpsLogicBegin();
void gpsLogicEnable();
void gpsLogicDisable();
void gpsLogicUpdate();
void gpsLogicReset();

void gpsLogicSetUpdateInterval(uint16_t intervalMs);

#endif // GPS_LOGIC_H
