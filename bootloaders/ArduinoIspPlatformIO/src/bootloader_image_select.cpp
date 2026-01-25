#include "bootloader_image_select.h"
#include "bootloader_burn.h"
#include <avr/pgmspace.h>

// 这里假设 a/b 的数组和长度变量已在别处定义
const boot_image_t g_boot_images[] = {
  { "2560-boot-original", boot_2560_a, boot_2560_a_len },
  { "2560-boot-recovery", boot_2560_b, boot_2560_b_len },
};

const uint8_t g_boot_images_count = sizeof(g_boot_images)/sizeof(g_boot_images[0]);

bool burn_selected_bootloader(uint8_t idx) {
  if (idx >= g_boot_images_count) {
    LOGLN("[OFFLINE ISP] ERROR: invalid image index");
    return false;
  }
  const boot_image_t *bi = &g_boot_images[idx];
  return burn_bootloader_2560_with_log_ex(bi->image, bi->len, bi->name);
}
