#pragma once

#include <Arduino.h>

void isp_read_signature(uint8_t sig[3]);

bool burn_bootloader_2560_with_log_ex(const uint8_t *image,
                                      uint32_t image_len,
                                      const char *name);