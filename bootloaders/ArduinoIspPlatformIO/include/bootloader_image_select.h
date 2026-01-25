#pragma once

#include "isp_config.h"


#include "bootloader_image_original_v2.h"
#include "bootloader_image_recovery_v2.h"

typedef struct {
  const char *name;
  const uint8_t *image;   // PROGMEM 数组首地址
  unsigned int len;
} boot_image_t;

// 你提供的多个镜像声明（每个镜像一个 .h/.c 生成的 PROGMEM 数组）
extern const uint8_t boot_2560_a[] PROGMEM;
extern const unsigned int boot_2560_a_len;

extern const uint8_t boot_2560_b[] PROGMEM;
extern const unsigned int boot_2560_b_len;

// 镜像表
extern const boot_image_t g_boot_images[];
extern const uint8_t g_boot_images_count;

bool burn_selected_bootloader(uint8_t idx);