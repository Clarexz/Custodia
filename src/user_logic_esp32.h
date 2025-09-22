/*
 * USER_LOGIC.H - GPIO Pin Definitions for XIAO ESP32S3
 * 
 * BLOCK A: One-MCU Unified Firmware
 * Hardware: XIAO ESP32S3 + Wio SX1262 + L76K GPS + MAX7219 LED Matrix
 * 
 */

#ifndef USER_LOGIC_ESP32_H
#define USER_LOGIC_ESP32_H

/*
 * ALREADY OCCUPIED PINS - DO NOT USE
 */

// GPS Module L76K (UART1)
#define GPS_RX_PIN              44          // Connected to GPS TXD
#define GPS_TX_PIN              43          // Connected to GPS RXD

// Meshtastic Communication (UART2) 
#define MESH_RX_PIN             2           // Connected to Meshtastic TX
#define MESH_TX_PIN             3           // Connected to Meshtastic RX

// LED Matrix MAX7219
#define LED_MATRIX_DATA_PIN     4           // Data line
#define LED_MATRIX_CS_PIN       5           // Chip Select
#define LED_MATRIX_CLK_PIN      6           // Clock line

// LoRa SX1262 Module
#define LORA_SCK_PIN            7           // SPI Clock
#define LORA_MISO_PIN           8           // SPI MISO
#define LORA_MOSI_PIN           9           // SPI MOSI
#define LORA_NSS_PIN            41          // Chip Select
#define LORA_DIO1_PIN           39          // Digital I/O 1
#define LORA_NRST_PIN           42          // Reset
#define LORA_BUSY_PIN           40          // Busy

// Status LED (used by all roles)
#define LED_PIN                 21          // Status LED indicator

// System Reserved
#define BOOT_PIN                0           // Boot button
#define USB_DM_PIN              19          // USB D-
#define USB_DP_PIN              20          // USB D+

/*
 * AVAILABLE PINS FOR USER LOGIC
 */

// Available GPIO Pins
#define USER_GPIO_1             1
#define USER_GPIO_10            10
#define USER_GPIO_11            11
#define USER_GPIO_12            12
#define USER_GPIO_13            13
#define USER_GPIO_14            14
#define USER_GPIO_15            15
#define USER_GPIO_16            16
#define USER_GPIO_17            17
#define USER_GPIO_18            18

// ADC Capable Pins (available for analog reading)
#define USER_ADC_1              1           // ADC1_CH0
#define USER_ADC_10             10          // ADC1_CH9 - Best option for analog

#endif // USER_LOGIC_ESP32_H
