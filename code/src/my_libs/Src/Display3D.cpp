#include "Display3D.hpp"
#include "infrare.h"
#include <Arduino.h>

extern _Infrare Infrare;

/* ===================== 协议常量 ===================== */



/* ===================== 底层发送 ===================== */

static void Display3D_Send(uint8_t cmd,
                        uint8_t d1,
                        uint8_t d2,
                        uint8_t d3,
                        uint8_t d4,
                        uint8_t repeat = 1)
{
    uint8_t frame[6] = { DISP3D_HEAD, cmd, d1, d2, d3, d4 };
    for (uint8_t i = 0; i < repeat; i++) {
        Infrare.Transmition_New(frame, 6);
        delay(75);
    }
}

/* ===================== UI ===================== */

void Display3D_UI_SetColorRGB(uint8_t r, uint8_t g, uint8_t b)
{
    Display3D_Send(DISP3D_CMD_SET_TEXT_RGB, DISP3D_RGB_TEXT_MODE, r, g, b, 2);
}

void Display3D_UI_SetTextColor(Display3DColor color)
{
    static const uint8_t table[][3] = {
        {255,0,0},{0,255,0},{0,0,255},{255,255,0},
        {255,0,255},{0,255,255},{0,0,0},{255,255,255}
    };

    if (color >= 1 && color <= 8) {
        Display3D_UI_SetColorRGB(
            table[color-1][0],
            table[color-1][1],
            table[color-1][2]
        );
    }
}

/* ===================== 预设显示 ===================== */

static void Display3D_ShowSimple(uint8_t cmd, uint8_t val)
{
    Display3D_Send(cmd, val, 0, 0, 0, 2);
}

void Display3D_Info_ShowShape(Display3DShape s) { Display3D_ShowSimple(DISP3D_CMD_SHOW_SHAPE, s); }
void Display3D_Info_ShowRoad (Display3DRoad  r) { Display3D_ShowSimple(DISP3D_CMD_SHOW_ROAD,  r); }
void Display3D_Info_ShowSign (Display3DSign  s) { Display3D_ShowSimple(DISP3D_CMD_SHOW_SIGN,  s); }


// 自定义文本（ASCII / GBK 自动）
void Display3D_Data_WriteText(const char* text)
{
    if (!text) return;

    int i = 0;
    while (text[i]) {
        uint8_t hi = (uint8_t)text[i++];
        uint8_t lo = 0x00;

        if ((hi & 0x80) && text[i]) {
            lo = (uint8_t)text[i++];
        }

        bool last = (text[i] == '\0');

        Display3D_Send(
            DISP3D_CMD_TEXT,
            hi,
            lo,
            last ? DISP3D_TEXT_LADISP3D_FLAG : DISP3D_TEXT_MORE_FLAG,
            0x00,
            1
        );

        delay(110);
    }
}

// 直接发 GBK
void Display3D_Data_WriteGBK(const uint16_t* gbk, int count)
{
    if (!gbk || count <= 0) return;

    for (int i = 0; i < count; i++) {
        bool last = (i == count - 1);
        Display3D_Send(
            DISP3D_CMD_TEXT,
            (uint8_t)(gbk[i] >> 8),
            (uint8_t)(gbk[i] & 0xFF),
            last ? DISP3D_TEXT_LADISP3D_FLAG : DISP3D_TEXT_MORE_FLAG,
            0x00,
            1
        );
        delay(110);
    }
}

// 距离显示
void Display3D_Data_WriteDist(uint16_t mm)
{
    uint16_t cm = mm / 10;
    uint8_t t = (cm / 10) % 10 + '0';
    uint8_t u = cm % 10 + '0';
    Display3D_Send(DISP3D_CMD_SHOW_DIST, t, u, 0, 0, 1);
}

void Display3D_Data_WritePlate(const char* plate6, uint8_t x, uint8_t y)
{
    if (!plate6) return;

    // 必须至少 6 个字符
    for (int i = 0; i < 6; i++) {
        if (plate6[i] == '\0') return;
    }

    // 先发前 4 位：0x20 车牌[1..4]
    Display3D_Send(
        DISP3D_CMD_PLATE_1_4,
        (uint8_t)plate6[0],
        (uint8_t)plate6[1],
        (uint8_t)plate6[2],
        (uint8_t)plate6[3],
        2
    );

    delay(150);

    // 再发后 2 位 + 坐标：0x10 车牌[5..6] + x + y
    Display3D_Send(
        DISP3D_CMD_PLATE_5_6_XY,
        (uint8_t)plate6[4],
        (uint8_t)plate6[5],
        x,
        y,
        2
    );
}
