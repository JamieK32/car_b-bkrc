#pragma once

#include <Arduino.h>

// Logging helpers.
#define LOG_ON 0

#ifdef SERIAL
#undef SERIAL
#endif

#ifdef SERIAL_PORT_USBVIRTUAL
#define SERIAL SERIAL_PORT_USBVIRTUAL
#else
#define SERIAL Serial
#endif

#if LOG_ON
#define LOG(s)    do { SERIAL.print(s); } while (0)
#define LOGLN(s)  do { SERIAL.println(s); } while (0)
#define LOGHEX2(b) do { if ((b) < 16) SERIAL.print('0'); SERIAL.print((b), HEX); } while (0)
#else
#define LOG(s)    do {} while (0)
#define LOGLN(s)  do {} while (0)
#define LOGHEX2(b) do {} while (0)
#endif

// ISP behavior.
#define PROG_FLICKER true

// Configure SPI clock (in Hz).
#define SPI_CLOCK (1000000 / 6)

// Serial baud rate.
#define BAUDRATE 19200

// Pin configuration.
#ifndef ARDUINO_HOODLOADER2
#define RESET 10
#define LED_HB 9
#define LED_ERR 8
#define LED_PMODE 7

// Uncomment to use Uno-style wiring on boards with separate ICSP header.
// #define USE_OLD_STYLE_WIRING

#ifdef USE_OLD_STYLE_WIRING
#define ARDUINOISP_PIN_MOSI 11
#define ARDUINOISP_PIN_MISO 12
#define ARDUINOISP_PIN_SCK 13
#endif

#else
#define RESET 4
#define LED_HB 7
#define LED_ERR 6
#define LED_PMODE 5
#endif

#ifndef ARDUINOISP_PIN_MOSI
#define ARDUINOISP_PIN_MOSI MOSI
#endif

#ifndef ARDUINOISP_PIN_MISO
#define ARDUINOISP_PIN_MISO MISO
#endif

#ifndef ARDUINOISP_PIN_SCK
#define ARDUINOISP_PIN_SCK SCK
#endif
