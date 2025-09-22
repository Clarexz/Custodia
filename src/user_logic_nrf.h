/*
 * USER_LOGIC_NRF.H - GPIO Pin Definitions for XIAO nRF52840 + SX1262
 *
 * Hardware: XIAO nRF52840 (Sense) + Wio SX1262 LoRa module
 *
 */

#ifndef USER_LOGIC_NRF_H
#define USER_LOGIC_NRF_H

// =====================
// Pin Mapping for Seeed XIAO nRF52840 + Wio SX1262
// =====================

// LoRa SX1262 Connections (matches Meshtastic seeed_xiao_nrf52840_kit variant)
#define LORA_NSS_PIN        D4
#define LORA_NRST_PIN       D2
#define LORA_DIO1_PIN       D1
#define LORA_BUSY_PIN       D3

#define LORA_MOSI_PIN       D10
#define LORA_MISO_PIN       D9
#define LORA_SCK_PIN        D8

// RXEN / RF switch control
#define LORA_RF_SW_PIN      D5

// Serial / Debug (USB CDC)
#define PIN_SERIAL_TX       TX
#define PIN_SERIAL_RX       RX

// I2C Pins
#define PIN_WIRE_SDA        SDA
#define PIN_WIRE_SCL        SCL

// Built-in LED
#define LED_PIN             LED_BUILTIN

// GPS placeholders (not connected in this setup)
#define GPS_RX_PIN          -1
#define GPS_TX_PIN          -1

#endif // USER_LOGIC_NRF_H
