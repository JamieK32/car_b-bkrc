#ifndef BUTTON_APP_H__
#define BUTTON_APP_H__

#include "multi_button.h"
#include <avr/io.h>

typedef enum {
	KEY_A = 0,
	KEY_B,
	KEY_C,
	KEY_D,
	BUTTON_NUM,
} BUTTON_ID;

void muti_button_init(BtnCallback single_click_cb);
extern struct Button buttons[BUTTON_NUM];

#endif
