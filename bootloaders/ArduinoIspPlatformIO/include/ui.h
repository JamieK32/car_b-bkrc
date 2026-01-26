#pragma once
#include <Arduino.h>
#include <U8g2lib.h>

#ifndef UI_W
#define UI_W 72
#endif

#ifndef UI_H
#define UI_H 40
#endif

typedef enum {
  UI_EVT_NONE = 0,
  UI_EVT_CONFIRM_A,
  UI_EVT_CONFIRM_B,
} ui_event_t;

void ui_init(U8G2 &disp, uint8_t pin_key_prev, uint8_t pin_key_ok);

void ui_set_viewport_offset(int8_t xOffset, int8_t yOffset);
void ui_get_viewport_offset(int8_t &xOffset, int8_t &yOffset);

void ui_set_firmware_labels(const char *labelA, const char *labelB);

uint8_t ui_get_selected(void);
void ui_set_selected(uint8_t idx);

void ui_tick(void);      // OneButton tick
void ui_render(void);    // UI页面（选择界面）
ui_event_t ui_pop_event(void);

// 72x40 状态页（1~3行）
void ui_draw_status(const char *line1, const char *line2, const char *line3);
