#ifndef USER_LOGIC_H
#define USER_LOGIC_H

#if defined(ARDUINO_ARCH_ESP32)
#include "user_logic_esp32.h"
#elif defined(SEEED_SOLAR_NODE)
#include "user_logic_solarnode.h"
#elif defined(NRF52_SERIES)
#include "user_logic_nrf.h"
#else
#error "Unsupported platform for user logic pin mapping"
#endif

#endif // USER_LOGIC_H
