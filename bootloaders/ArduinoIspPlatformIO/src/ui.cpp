#include "ui.h"
#include <OneButton.h>

// -------- 内部状态 --------
static U8G2 *g = nullptr;
static uint8_t g_pin_prev = 0;
static uint8_t g_pin_ok   = 0;

static int8_t g_off_x = 28;
static int8_t g_off_y = 12;

static const char *g_labelA = "BootA";
static const char *g_labelB = "BootB";

static uint8_t g_selected = 0;     // 0=A 1=B
static ui_event_t g_evt = UI_EVT_NONE;

// OneButton（使用指针，便于在 ui_init 里构造）
static OneButton *btnPrev = nullptr;
static OneButton *btnOk   = nullptr;

// -------- 工具函数 --------
static inline int16_t X(int16_t x) { return (int16_t)x + g_off_x; }
static inline int16_t Y(int16_t y) { return (int16_t)y + g_off_y; }

static inline void begin_viewport() {
  g->setClipWindow(g_off_x, g_off_y, g_off_x + UI_W, g_off_y + UI_H);
}
static inline void end_viewport() {
  g->setMaxClipWindow();
}

static void draw_center_str(int16_t cx, int16_t y, const char *s) {
  int16_t w = g->getStrWidth(s);
  g->drawStr(cx - w / 2, y, s);
}

// -------- OneButton 回调（短按 click）--------
static void on_prev_click() {
  g_selected ^= 1;
}

static void on_ok_click() {
  g_evt = (g_selected == 0) ? UI_EVT_CONFIRM_A : UI_EVT_CONFIRM_B;
}

// -------- 对外接口 --------
void ui_init(U8G2 &disp, uint8_t pin_key_prev, uint8_t pin_key_ok) {
  g = &disp;
  g_pin_prev = pin_key_prev;
  g_pin_ok   = pin_key_ok;

  // OneButton 内部也会处理去抖；这里仍然用上拉输入
  pinMode(g_pin_prev, INPUT_PULLUP);
  pinMode(g_pin_ok,   INPUT_PULLUP);

  // activeLow=true（按下为LOW），pullupActive=true（使用内部上拉）
  // 注意：不同版本 OneButton 构造函数参数略有差异；下面这一种是常见写法：
  // OneButton(pin, activeLow, pullupActive)
  btnPrev = new OneButton(g_pin_prev, true, true);
  btnOk   = new OneButton(g_pin_ok,   true, true);

  // 只要短按：click
  btnPrev->attachClick(on_prev_click);
  btnOk->attachClick(on_ok_click);

  g_selected = 0;
  g_evt = UI_EVT_NONE;
}

void ui_set_viewport_offset(int8_t xOffset, int8_t yOffset) {
  g_off_x = xOffset;
  g_off_y = yOffset;
}

void ui_get_viewport_offset(int8_t &xOffset, int8_t &yOffset) {
  xOffset = g_off_x;
  yOffset = g_off_y;
}

void ui_set_firmware_labels(const char *labelA, const char *labelB) {
  if (labelA) g_labelA = labelA;
  if (labelB) g_labelB = labelB;
}

uint8_t ui_get_selected(void) {
  return g_selected;
}

void ui_set_selected(uint8_t idx) {
  g_selected = (idx ? 1 : 0);
}

ui_event_t ui_pop_event(void) {
  ui_event_t e = g_evt;
  g_evt = UI_EVT_NONE;
  return e;
}

void ui_tick(void) {
  if (!g) return;
  if (btnPrev) btnPrev->tick();
  if (btnOk)   btnOk->tick();
}

void ui_render(void) {
  if (!g) return;

  // 页缓冲（_1_/_2_）必须 firstPage/nextPage
  g->firstPage();
  do {
    begin_viewport();

    // 清窗口
    g->setDrawColor(0);
    g->drawBox(X(0), Y(0), UI_W, UI_H);
    g->setDrawColor(1);

    // 框
    g->drawFrame(X(0), Y(0), UI_W, UI_H);

    // 标题
    g->setFont(u8g2_font_5x8_tr);
    draw_center_str(X(UI_W / 2), Y(9), "ABrobot");

    // 提示短字
    g->drawStr(X(2), Y(18), "D3 OK D4 Sel");

    // 两项
    const int16_t y1 = 28;
    const int16_t y2 = 38;

    if (g_selected == 0) g->drawStr(X(2), Y(y1), ">");
    else                 g->drawStr(X(2), Y(y2), ">");

    g->drawStr(X(10), Y(y1), g_labelA);
    g->drawStr(X(10), Y(y2), g_labelB);

    end_viewport();
  } while (g->nextPage());
}

void ui_draw_status(const char *line1, const char *line2, const char *line3) {
  if (!g) return;

  g->firstPage();
  do {
    begin_viewport();

    g->setDrawColor(0);
    g->drawBox(X(0), Y(0), UI_W, UI_H);
    g->setDrawColor(1);

    g->drawFrame(X(0), Y(0), UI_W, UI_H);
    g->setFont(u8g2_font_5x8_tr);

    if (line1) g->drawStr(X(3), Y(10), line1);
    if (line2) g->drawStr(X(3), Y(22), line2);
    if (line3) g->drawStr(X(3), Y(34), line3);

    end_viewport();
  } while (g->nextPage());
}
