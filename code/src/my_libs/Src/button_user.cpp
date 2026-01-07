#include "button_user.h"


#define KEY_ACTIVE_LEVEL 1

struct Button buttons[BUTTON_NUM]; 

uint8_t button_ids[BUTTON_NUM] = {KEY_A, KEY_B, KEY_C, KEY_D};

static uint8_t read_key0(void) { return ((PINK & (1<<0)) == 0); } // 按下=1
static uint8_t read_key1(void) { return ((PINK & (1<<1)) == 0); }
static uint8_t read_key2(void) { return ((PINK & (1<<2)) == 0); }
static uint8_t read_key3(void) { return ((PINK & (1<<3)) == 0); }



/* 你的按钮读取函数：适配 button_init 的 read_cb */
static inline uint8_t read_button_GPIO(uint8_t button_id)
{
    switch (button_id)
    {
        case KEY_A: return read_key0();
        case KEY_B: return read_key1();
        case KEY_C: return read_key2();
        case KEY_D: return read_key3();
        default:    return 0;
    }
}

void muti_button_init(BtnCallback single_click_cb)
{
    PORTK &= 0xf0;
    DDRK  &= 0xf0;
    for (int i = 0; i < BUTTON_NUM; i++)
    {
        button_init(&buttons[i], read_button_GPIO, KEY_ACTIVE_LEVEL, button_ids[i]);
        button_attach(&buttons[i], SINGLE_CLICK, single_click_cb);
        button_start(&buttons[i]);
    }
}
