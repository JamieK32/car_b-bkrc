// Offline ISP for ATmega2560 bootloader + OLED UI select (A/B)

#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

#include "isp_config.h"
#include "isp_spi.h"
#include "bootloader_burn.h"
#include "bootloader_image_select.h"
#include "ui.h"

// ---------------- OLED ----------------
U8G2_SSD1306_128X64_NONAME_1_HW_I2C  u8g2(U8G2_R0, A5, A4, U8X8_PIN_NONE);

// ---------------- 状态机 ----------------
enum AppState : uint8_t {
  ST_UI = 0,
  ST_BURN,
  ST_RESULT
};

static AppState g_state = ST_UI;
static bool     g_last_burn_ok = false;
static uint8_t  g_last_choice  = 0;

// 结果页闪灯（非阻塞）
static uint32_t g_blink_t = 0;
static bool g_led = false;

// ---------------- ISP烧录流程（阻塞） ----------------
static bool do_burn(uint8_t idx) {
  ui_draw_status("ABrobot", "PMODE", "...");
  LOGLN("\r\n[OFFLINE ISP] Enter program mode...");
  isp_start_pmode();

  uint8_t sig[3] = {0};
  isp_read_signature(sig);

  char sline[20];
  snprintf(sline, sizeof(sline), "%02X%02X%02X", sig[0], sig[1], sig[2]);
  ui_draw_status("SIG", sline, nullptr);

  bool ok = false;

  // ATmega2560: 1E 98 01
  if (sig[0] == 0x1E && sig[1] == 0x98 && sig[2] == 0x01) {
    digitalWrite(LED_PMODE, HIGH);

    if (idx == 0) ui_draw_status("BURN", "BootA", "WAIT");
    else          ui_draw_status("BURN", "BootB", "WAIT");

    ok = burn_selected_bootloader(idx);
  } else {
    ui_draw_status("ERR", "TARGET", "ABORT");
    ok = false;
  }

  isp_end_pmode();
  digitalWrite(LED_PMODE, LOW);
  return ok;
}

// ---------------- 进入状态 ----------------
static void enter_state(AppState s) {
  g_state = s;

  if (s == ST_UI) {
    digitalWrite(LED_ERR, LOW);
    ui_draw_status("ABrobot", "Select", "D3/D4");
    delay(80);
  } else if (s == ST_RESULT) {
    if (g_last_burn_ok) {
      ui_draw_status("OK", "DONE", "D3 BACK");
      digitalWrite(LED_ERR, LOW);
    } else {
      ui_draw_status("FAIL", "CHECK", "D3 BACK");
      digitalWrite(LED_ERR, HIGH);
    }
    g_blink_t = millis();
    g_led = false;
    digitalWrite(13, LOW);
  }
}

void setup() {
  SERIAL.begin(BAUDRATE);

#if defined(USBCON) || defined(ARDUINO_ARCH_SAMD) || defined(ARDUINO_ARCH_MBED)
  unsigned long t0 = millis();
  while (!SERIAL && (millis() - t0 < 1500)) { ; }
#endif

  pinMode(LED_PMODE, OUTPUT);
  pinMode(LED_ERR, OUTPUT);
  pinMode(LED_HB, OUTPUT);
  pinMode(13, OUTPUT);

  digitalWrite(LED_PMODE, LOW);
  digitalWrite(LED_ERR, LOW);
  digitalWrite(LED_HB, LOW);
  digitalWrite(13, LOW);

  // OLED
  u8g2.begin();
  u8g2.setContrast(255);

  // UI：D3=确认 D4=选择
  ui_init(u8g2, 3, 4);

  // 72x40 偏移：按你屏幕实际区域调整
  ui_set_viewport_offset(30, 12);

  // 短字
  ui_set_firmware_labels("original", "recovery");

  enter_state(ST_UI);
}

void loop() {
  switch (g_state) {
    case ST_UI: {
      ui_tick();
      ui_render();

      ui_event_t e = ui_pop_event();
      if (e == UI_EVT_CONFIRM_A || e == UI_EVT_CONFIRM_B) {
        g_last_choice = (e == UI_EVT_CONFIRM_A) ? 0 : 1;
        g_state = ST_BURN;
      }
    } break;

    case ST_BURN: {
      ui_draw_status("ABrobot", "READY", nullptr);
      delay(150);

      g_last_burn_ok = do_burn(g_last_choice);

      enter_state(ST_RESULT);
    } break;

    case ST_RESULT: {
      // 闪灯
      uint16_t period = g_last_burn_ok ? 450 : 160;
      if (millis() - g_blink_t >= period) {
        g_blink_t = millis();
        g_led = !g_led;
        digitalWrite(13, g_led ? HIGH : LOW);
      }

      // 返回 UI：只要按一次 D4（OK click 会产生 confirm 事件）
      ui_tick();
      ui_event_t e = ui_pop_event();
      if (e != UI_EVT_NONE) {
        enter_state(ST_UI);
      }
    } break;
  }

  delay(1);
}
