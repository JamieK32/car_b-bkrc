#include "bootloader_burn.h"

#include "bootloader_image_select.h"
#include "isp_config.h"
#include "isp_spi.h"

#define PTIME 30

static unsigned int here;

static void prog_lamp(int state) {
  if (PROG_FLICKER) {
    digitalWrite(LED_PMODE, state);
  }
}

static unsigned int current_page() {
  const unsigned int page_words = 256 / 2;
  return here & ~(page_words - 1);
}

static void flash(uint8_t hilo, unsigned int addr, uint8_t data) {
  isp_spi_transaction(0x40 + 8 * hilo,
                      (addr >> 8) & 0xFF,
                      addr & 0xFF,
                      data);
}

static void commit(unsigned int addr) {
  if (PROG_FLICKER) {
    prog_lamp(LOW);
  }
  isp_spi_transaction(0x4C, (addr >> 8) & 0xFF, addr & 0xFF, 0);
  if (PROG_FLICKER) {
    delay(PTIME);
    prog_lamp(HIGH);
  }
}

// 未使用：如果不需要可删除
static void chip_erase() {
  isp_spi_transaction(0xAC, 0x80, 0x00, 0x00);
  delay(50);
}

static uint16_t read_flash_word(uint32_t word_addr) {
  uint8_t low  = isp_spi_transaction(0x20, (word_addr >> 8) & 0xFF, word_addr & 0xFF, 0);
  uint8_t high = isp_spi_transaction(0x28, (word_addr >> 8) & 0xFF, word_addr & 0xFF, 0);
  return (uint16_t)((uint16_t)high << 8) | low;
}

void isp_read_signature(uint8_t sig[3]) {
  sig[0] = isp_spi_transaction(0x30, 0x00, 0x00, 0x00);
  sig[1] = isp_spi_transaction(0x30, 0x00, 0x01, 0x00);
  sig[2] = isp_spi_transaction(0x30, 0x00, 0x02, 0x00);
}

bool burn_bootloader_2560_with_log_ex(const uint8_t *image,
                                      uint32_t image_len,
                                      const char *name) {
  (void)name; // 未使用，避免编译告警

  const uint32_t FLASH_BYTES = 0x40000UL;   // ATmega2560
  const uint32_t BOOT_BYTES  = 0x2000UL;    // 8KB boot section
  const uint32_t BOOT_START_BYTE = FLASH_BYTES - BOOT_BYTES;
  const uint32_t start_word = BOOT_START_BYTE / 2;

  if (image_len > BOOT_BYTES) {
    return false;
  }

  chip_erase();

  here = (unsigned int)start_word;

  uint32_t i = 0;
  unsigned int page = current_page();

  while (i < image_len) {
    if (page != current_page()) {
      commit(page);
      page = current_page();
    }

    uint8_t low  = pgm_read_byte(&image[i++]);
    uint8_t high = 0xFF;
    if (i < image_len) {
      high = pgm_read_byte(&image[i++]);
    }

    flash(LOW,  here, low);
    flash(HIGH, here, high);
    here++;
  }

  commit(page);

  // Verify (first 64 words)
  for (uint16_t w = 0; w < 64; w++) {
    uint16_t got = read_flash_word(start_word + w);

    uint8_t low  = pgm_read_byte(&image[w * 2]);
    uint8_t high = pgm_read_byte(&image[w * 2 + 1]);
    uint16_t exp = (uint16_t)((uint16_t)high << 8) | low;

    if (got != exp) {
      return false;
    }
  }

  return true;
}
