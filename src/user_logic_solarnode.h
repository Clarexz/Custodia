/*
 * USER_LOGIC_SOLARNODE.H - GPIO Pin Definitions for SenseCAP Solar Node P1-Pro
 *
 * Hardware: SenseCAP Solar Node P1-Pro (nRF52840) + SX1262 + L76K GNSS
 */

#ifndef USER_LOGIC_SOLARNODE_H
#define USER_LOGIC_SOLARNODE_H

// =====================
// LoRa SX1262 Connections (matches Meshtastic seeed_solar_node variant)
// =====================
#define LORA_NSS_PIN        D4
#define LORA_NRST_PIN       D2
#define LORA_DIO1_PIN       D1
#define LORA_BUSY_PIN       D3

#define LORA_MOSI_PIN       D10
#define LORA_MISO_PIN       D9
#define LORA_SCK_PIN        D8

#define LORA_RF_SW_PIN      D5    // Acts as RX enable / RF switch control

// =====================
// Serial / Debug (USB CDC)
// =====================
#define PIN_SERIAL_TX       TX
#define PIN_SERIAL_RX       RX

// =====================
// I2C (Grove connector)
// =====================
#define PIN_WIRE_SDA        D14
#define PIN_WIRE_SCL        D15

// =====================
// Built-in LEDs
// =====================
#define LED_PIN             LED_BUILTIN
#define LED_CONN_PIN        LED_CONN

// =====================
// GPS (L76K) Connections
// =====================
#define GPS_RX_PIN          D6    // MCU RX <- GNSS TX
#define GPS_TX_PIN          D7    // MCU TX -> GNSS RX
#define GPS_STANDBY_PIN     D0    // GNSS wake-up
#define GPS_ENABLE_PIN      D18   // GNSS enable
#define GPS_RESET_PIN       D17   // GNSS reset

// =====================
// Power / Battery Monitoring
// =====================
#define BATTERY_MONITOR_PIN PIN_VBAT
#define BATTERY_READ_PIN    D19

#endif // USER_LOGIC_SOLARNODE_H

