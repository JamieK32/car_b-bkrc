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
                      addr >> 8 & 0xFF,
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

static void chip_erase() {
  isp_spi_transaction(0xAC, 0x80, 0x00, 0x00);
  delay(50);
}

static uint16_t read_flash_word(uint32_t word_addr) {
  uint8_t low = isp_spi_transaction(0x20, (word_addr >> 8) & 0xFF, word_addr & 0xFF, 0);
  uint8_t high = isp_spi_transaction(0x28, (word_addr >> 8) & 0xFF, word_addr & 0xFF, 0);
  return (uint16_t)high << 8 | low;
}

void isp_read_signature(uint8_t sig[3]) {
  sig[0] = isp_spi_transaction(0x30, 0x00, 0x00, 0x00);
  sig[1] = isp_spi_transaction(0x30, 0x00, 0x01, 0x00);
  sig[2] = isp_spi_transaction(0x30, 0x00, 0x02, 0x00);
}

bool burn_bootloader_2560_with_log_ex(const uint8_t *image,
                                      uint32_t image_len,
                                      const char *name) {
  const uint32_t FLASH_BYTES = 0x40000UL;   // ATmega2560
  const uint32_t BOOT_BYTES  = 0x2000UL;    // 8KB boot section
  const uint32_t BOOT_START_BYTE = FLASH_BYTES - BOOT_BYTES;
  const uint32_t start_word = BOOT_START_BYTE / 2;

  LOGLN("[OFFLINE ISP] Start burning bootloader...");
  if (name) {
    LOG("[OFFLINE ISP] image=");
    LOGLN(name);
  }
  LOG("[OFFLINE ISP] image_len=");
  SERIAL.println(image_len);
  LOG("[OFFLINE ISP] BOOT_START=0x");
  SERIAL.println(BOOT_START_BYTE, HEX);

  if (image_len > BOOT_BYTES) {
    LOGLN("[OFFLINE ISP] ERROR: boot image too large for boot section");
    return false;
  }

  LOGLN("[OFFLINE ISP] Chip erase...");
  chip_erase();
  LOGLN("[OFFLINE ISP] Chip erase done");

  here = (unsigned int)start_word;

  uint32_t i = 0;
  unsigned int page = current_page();

  const uint32_t prog_step = 512;
  uint32_t next_report = prog_step;

  while (i < image_len) {
    if (page != current_page()) {
      commit(page);
      page = current_page();
    }

    // ★这里改成从参数 image 读
    uint8_t low  = pgm_read_byte(&image[i++]);
    uint8_t high = 0xFF;
    if (i < image_len) {
      high = pgm_read_byte(&image[i++]);
    }

    flash(LOW,  here, low);
    flash(HIGH, here, high);
    here++;

    if (i >= next_report || i == image_len) {
      LOG("[OFFLINE ISP] Writing... ");
      SERIAL.print((i * 100UL) / image_len);
      LOGLN("%");
      next_report += prog_step;
    }
  }

  commit(page);
  LOGLN("[OFFLINE ISP] Write done");

  LOGLN("[OFFLINE ISP] Verify (first 64 words)...");
  for (uint16_t w = 0; w < 64; w++) {
    uint16_t got = read_flash_word(start_word + w);

    // ★这里也改成从参数 image 读
    uint8_t low  = pgm_read_byte(&image[w * 2]);
    uint8_t high = pgm_read_byte(&image[w * 2 + 1]);
    uint16_t exp = ((uint16_t)high << 8) | low;

    if (got != exp) {
      LOG("[OFFLINE ISP] VERIFY FAIL at word ");
      SERIAL.println(w);
      LOG("[OFFLINE ISP] got=0x");
      SERIAL.println(got, HEX);
      LOG("[OFFLINE ISP] exp=0x");
      SERIAL.println(exp, HEX);
      return false;
    }
  }
  LOGLN("[OFFLINE ISP] Verify OK");
  return true;
}