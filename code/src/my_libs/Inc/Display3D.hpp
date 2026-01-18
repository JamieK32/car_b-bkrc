#ifndef __3D_DISPLAY_H__
#define __3D_DISPLAY_H__

#include <stdint.h>


/* ===================== 协议常量 ===================== */

#define DISP3D_HEAD                 0xFF

// 主指令（CMD）
#define DISP3D_CMD_TEXT             0x31
#define DISP3D_CMD_SHOW_DIST        0x11
#define DISP3D_CMD_SHOW_SHAPE       0x12
#define DISP3D_CMD_SHOW_ROAD        0x14
#define DISP3D_CMD_SHOW_SIGN        0x15
#define DISP3D_CMD_SET_TEXT_RGB     0x17
#define DISP3D_CMD_PLATE_1_4        0x20
#define DISP3D_CMD_PLATE_5_6_XY     0x10
#define DISP3D_RGB_TEXT_MODE        0x01

// 文本协议控制字
#define DISP3D_TEXT_LADISP3D_FLAG   0x55   // 最后一帧标志
#define DISP3D_TEXT_MORE_FLAG       0x00   // 后续还有字符


/* ===================== 图形 ===================== */

typedef enum {
    DISP3D_SHAPE_RECT    = 0x01,
    DISP3D_SHAPE_CIRCLE  = 0x02,
    DISP3D_SHAPE_TRI     = 0x03,
    DISP3D_SHAPE_DIAMOND = 0x04,
    DISP3D_SHAPE_STAR    = 0x05
} Display3DShape;

/* ===================== 颜色 ===================== */

typedef enum {
    DISP3D_COLOR_RED     = 0x01,
    DISP3D_COLOR_GREEN   = 0x02,
    DISP3D_COLOR_BLUE    = 0x03,
    DISP3D_COLOR_YELLOW  = 0x04,
    DISP3D_COLOR_MAGENTA = 0x05,
    DISP3D_COLOR_CYAN    = 0x06,
    DISP3D_COLOR_BLACK   = 0x07,
    DISP3D_COLOR_WHITE   = 0x08
} Display3DColor;

/* ===================== 路况 ===================== */

typedef enum {
    DISP3D_ROAD_SCHOOL    = 0x01,
    DISP3D_ROAD_WORK      = 0x02,
    DISP3D_ROAD_DANGER    = 0x03,
    DISP3D_ROAD_KEEP_DIST = 0x04,
    DISP3D_ROAD_NO_DRINK  = 0x05,
    DISP3D_ROAD_NO_WASTE  = 0x06
} Display3DRoad;

/* ===================== 标志 ===================== */

typedef enum {
    DISP3D_SIGN_STRAIGHT = 0x01,
    DISP3D_SIGN_LEFT     = 0x02,
    DISP3D_SIGN_RIGHT    = 0x03,
    DISP3D_SIGN_UTURN    = 0x04,
    DISP3D_SIGN_NO_STRA  = 0x05,
    DISP3D_SIGN_NO_PASS  = 0x06
} Display3DSign;

/* ===================== API ===================== */

// UI
void Display3D_UI_SetColorRGB(uint8_t r, uint8_t g, uint8_t b);
void Display3D_UI_SetTextColor(Display3DColor color);

// 预设内容
void Display3D_Info_ShowShape(Display3DShape shape);
void Display3D_Info_ShowRoad (Display3DRoad road);
void Display3D_Info_ShowSign (Display3DSign sign);

// 数据显示
void Display3D_Data_WriteText(const char* text);
void Display3D_Data_WriteGBK8 (uint8_t* gbk, int count);
void Display3D_Data_WriteDist(uint16_t mm);

void Display3D_Data_WritePlate(const char* plate6, uint8_t x, uint8_t y);

#endif
