/*
 * GPS_MANAGER.H - Fachada ligera que expone los datos de gps_logic.cpp
 */

#ifndef GPS_MANAGER_H
#define GPS_MANAGER_H

#include <Arduino.h>
#include "gps_types.h"

class GPSManager {
public:
    GPSManager();
    ~GPSManager() = default;

    void begin();
    void update();
    void enable();
    void disable();
    void reset();

    void setUpdateInterval(uint16_t intervalMs);

    GPSData getCurrentData();
    GPSData* getCurrentDataPtr();
    float getLatitude();
    float getLongitude();
    bool hasValidFix();
    uint32_t getTimestamp();
    uint8_t getSatelliteCount();
    GPSStatus getStatus();
    String getStatusString();
    bool isEnabled();

    String formatCoordinates();
    String formatForTransmission();
    String formatPacketWithDeviceID(uint16_t deviceID);
    String latitudeToString(int precision = 6);
    String longitudeToString(int precision = 6);

    bool isValidLatitude(float lat);
    bool isValidLongitude(float lon);
    bool isValidCoordinate(float lat, float lon);
    float distanceTo(float lat, float lon);
    float bearingTo(float lat, float lon);

    void printCurrentData();
    void printStatus();
    void printSimulationInfo();
    uint32_t getTotalUpdates();
    unsigned long getUptimeSeconds();

private:
    uint16_t updateInterval;
    unsigned long startTime;
    uint32_t totalUpdates;
};

extern GPSManager gpsManager;

#endif // GPS_MANAGER_H
