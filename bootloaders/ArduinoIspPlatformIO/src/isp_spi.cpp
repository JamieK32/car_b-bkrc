#include "isp_spi.h"

#if defined(ARDUINO_ARCH_AVR)
#if SPI_CLOCK > (F_CPU / 128)
#define USE_HARDWARE_SPI
#endif
#endif

// Force bitbanged SPI if not using the hardware SPI pins.
#if (ARDUINOISP_PIN_MISO != MISO) || (ARDUINOISP_PIN_MOSI != MOSI) || (ARDUINOISP_PIN_SCK != SCK)
#undef USE_HARDWARE_SPI
#endif

#ifdef USE_HARDWARE_SPI
#include <SPI.h>
#else

#define SPI_MODE0 0x00

#if !defined(ARDUINO_API_VERSION) || ARDUINO_API_VERSION != 10001
class SPISettings {
public:
  SPISettings(uint32_t clock, uint8_t bitOrder, uint8_t dataMode)
    : clockFreq(clock) {
    (void)bitOrder;
    (void)dataMode;
  }

  uint32_t getClockFreq() const {
    return clockFreq;
  }

private:
  uint32_t clockFreq;
};
#endif

class BitBangedSPI {
public:
  void begin() {
    digitalWrite(ARDUINOISP_PIN_SCK, LOW);
    digitalWrite(ARDUINOISP_PIN_MOSI, LOW);
    pinMode(ARDUINOISP_PIN_SCK, OUTPUT);
    pinMode(ARDUINOISP_PIN_MOSI, OUTPUT);
    pinMode(ARDUINOISP_PIN_MISO, INPUT);
  }

  void beginTransaction(SPISettings settings) {
    pulseWidth = (500000 + settings.getClockFreq() - 1) / settings.getClockFreq();
    if (pulseWidth == 0) {
      pulseWidth = 1;
    }
  }

  void end() {}

  uint8_t transfer(uint8_t b) {
    for (unsigned int i = 0; i < 8; ++i) {
      digitalWrite(ARDUINOISP_PIN_MOSI, (b & 0x80) ? HIGH : LOW);
      digitalWrite(ARDUINOISP_PIN_SCK, HIGH);
      delayMicroseconds(pulseWidth);
      b = (b << 1) | digitalRead(ARDUINOISP_PIN_MISO);
      digitalWrite(ARDUINOISP_PIN_SCK, LOW);
      delayMicroseconds(pulseWidth);
    }
    return b;
  }

private:
  unsigned long pulseWidth;
};

static BitBangedSPI SPI;

#endif

static bool rst_active_high = false;

static void reset_target(bool reset) {
  digitalWrite(RESET, ((reset && rst_active_high) || (!reset && !rst_active_high)) ? HIGH : LOW);
}

uint8_t isp_spi_transaction(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
  SPI.transfer(a);
  SPI.transfer(b);
  SPI.transfer(c);
  return SPI.transfer(d);
}

void isp_start_pmode() {
  reset_target(true);
  pinMode(RESET, OUTPUT);

  SPI.begin();
  SPI.beginTransaction(SPISettings(SPI_CLOCK, MSBFIRST, SPI_MODE0));

  digitalWrite(ARDUINOISP_PIN_SCK, LOW);
  delay(20);
  reset_target(false);
  delayMicroseconds(100);
  reset_target(true);

  delay(50);
  isp_spi_transaction(0xAC, 0x53, 0x00, 0x00);
}

void isp_end_pmode() {
  SPI.end();
  pinMode(ARDUINOISP_PIN_MOSI, INPUT);
  pinMode(ARDUINOISP_PIN_SCK, INPUT);
  reset_target(false);
  pinMode(RESET, INPUT);
}
