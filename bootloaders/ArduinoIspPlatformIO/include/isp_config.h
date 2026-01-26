#pragma once

#include <Arduino.h>

// ISP behavior.
#define PROG_FLICKER true

// Configure SPI clock (in Hz).
#define SPI_CLOCK (1000000 / 6)

#define RESET 10
#define LED_HB 9
#define LED_ERR 8
#define LED_PMODE 7


#ifndef ARDUINOISP_PIN_MOSI
#define ARDUINOISP_PIN_MOSI MOSI
#endif

#ifndef ARDUINOISP_PIN_MISO
#define ARDUINOISP_PIN_MISO MISO
#endif

#ifndef ARDUINOISP_PIN_SCK
#define ARDUINOISP_PIN_SCK SCK
#endif
