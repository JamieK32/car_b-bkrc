//百科荣创官方库
#include "DCMotor.h"
#include "CoreLED.h"
#include "CoreBeep.h"
#include "ExtSRAMInterface.h"
#include "LED.h"
#include "BH1750.h"
#include "Command.h"
#include "BEEP.h"
#include "Drive.h"
#include "infrare.h"
#include "BKRC_Voice.h"

// C库
#include "string.h"
#include "math.h"

// 自己的库
#include "carb_config.h"
#include "car_controller.hpp"
#include "log.hpp"
#include "k210_protocol.hpp"
#include "k210_app.hpp"
#include "lsm6dsv16x.h"
#include "qrcode_datamap.hpp"
#include "algorithm.hpp"
#include "vehicle_exchange.h"
#include "street_light.hpp"
#include "zigbee_driver.hpp"
#include "Infrared_driver.hpp"
#include "Display3D.hpp"
#include "multi_button.h"
#include "button_user.h"
#include "qrcode_datamap.hpp"
#include "ultrasonic.hpp"
#include "vehicle_exchange_v2.h"

// 各种演示
#include "qrcode_demos.hpp"

#define LOG_MAIN_EN  1

#if LOG_MAIN_EN
  #define log_main(fmt, ...)  LOG_P("[MAIN] " fmt "\r\n", ##__VA_ARGS__)
  #define log_printf(fmt, ...)  LOG_P(fmt, ##__VA_ARGS__)
  #define alarm_fail()  do { BEEP.ToggleNTimes(3, 120); } while(0)
  #define alarm_log1()   do { BEEP.ToggleNTimes(1, 120); } while(0)
  #define alarm_log2()   do { BEEP.ToggleNTimes(2, 120); } while(0)
#else
  #define log_main(...) do {} while(0)
  #define alarm_fail()  do {} while(0)
  #define alarm_log1()   do {} while(0)
  #define alarm_log2()   do {} while(0)
#endif

/* ===================== 角色选择：两端分别改成 0/1 ===================== */
#ifndef IS_SIDE_A
#define IS_SIDE_A 0   // A端=1, B端=0
#endif

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>

#include "vehicle_exchange_v2.h"

typedef enum {
    ROLE_A = 0,
    ROLE_B = 1,
} TestRole;

/* ===================== 小工具：统一日志格式 ===================== */
static const char* role_str(TestRole r) { return (r == ROLE_A) ? "A" : "B"; }

static void log_line(TestRole role, const char *fmt, ...)
{
    /* 统一： [123456ms][A] ... */
    uint32_t ms = get_ms();
    log_printf("[%lu ms][%s] ", (unsigned long)ms, role_str(role));

    va_list ap;
    va_start(ap, fmt);
    /* 如果你的 log_printf 不支持 vprintf，这里就直接改成 log_printf(fmt, ...) 的方式 */
    /* 但大多数 RTT printf 封装都是支持的。若不支持，告诉我我再给你无va_list版本。 */
    /* 这里用一个最保险的办法：把内容先格式化到缓冲区 */
    char buf[256];
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    log_printf("%s\r\n", buf);
}

/* ===================== 漂亮打印HEX：16字节一行 ===================== */
static void dump_hex_block(TestRole role, const char *tag, const uint8_t *buf, uint8_t len)
{
    log_line(role, "%s (len=%u)", tag, (unsigned)len);

    for (uint8_t i = 0; i < len; i += 16) {
        char line[128];
        int pos = 0;

        /* 偏移 */
        pos += snprintf(line + pos, sizeof(line) - pos, "    %02u: ", (unsigned)i);

        /* 16字节 HEX */
        uint8_t chunk = (uint8_t)((len - i) > 16 ? 16 : (len - i));
        for (uint8_t j = 0; j < 16; j++) {
            if (j < chunk) pos += snprintf(line + pos, sizeof(line) - pos, "%02X ", buf[i + j]);
            else           pos += snprintf(line + pos, sizeof(line) - pos, "   ");
        }

        /* 可选：ASCII 视图（非可打印用点号） */
        pos += snprintf(line + pos, sizeof(line) - pos, " |");
        for (uint8_t j = 0; j < chunk; j++) {
            uint8_t c = buf[i + j];
            line[pos++] = (c >= 32 && c <= 126) ? (char)c : '.';
        }
        line[pos++] = '|';
        line[pos] = '\0';

        log_line(role, "%s", line);
    }
}

/* ===================== 单次互测（参数控制A/B） ===================== */
void log_main_single_test_once(TestRole role)
{
    if (role == ROLE_A) {
        /* A端：发 RFID -> 等 CMD */
        uint8_t rfid[16];
        for (uint8_t i = 0; i < 16; i++) rfid[i] = (uint8_t)(0xA0 + i);

        log_line(role, "TX -> ITEM_RFID (16 bytes) ...");
        bool ok = Zigbee_SendItemReliable(ITEM_RFID, rfid, 16, 0);
        log_line(role, "TX -> ITEM_RFID : %s", ok ? "OK" : "FAIL");

        uint8_t item_id = 0;
        uint8_t cmd_buf[1] = {0};
        uint8_t len = 1;

        log_line(role, "RX <- ITEM_CMD (1 byte) ...");
        ok = Zigbee_WaitItemReliable(&item_id, cmd_buf, &len, 0);
        if (ok) {
            log_line(role, "RX <- item_id=0x%02X OK", item_id);
            dump_hex_block(role, "CMD", cmd_buf, len);
        } else {
            log_line(role, "RX <- ITEM_CMD : FAIL");
        }

    } else {
        /* B端：等 RFID -> 回 CMD */
        uint8_t item_id = 0;
        uint8_t rfid[16] = {0};
        uint8_t len = 16;

        log_line(role, "RX <- ITEM_RFID (16 bytes) ...");
        bool ok = Zigbee_WaitItemReliable(&item_id, rfid, &len, 0);
        if (ok) {
            log_line(role, "RX <- item_id=0x%02X OK", item_id);
            dump_hex_block(role, "RFID", rfid, len);
        } else {
            log_line(role, "RX <- ITEM_RFID : FAIL");
        }

        /* 测试更稳：给A端一点时间从“发”切到“收”（避免双向抢空口） */
        delay_ms(150);

        uint8_t cmd = 0x5A;
        log_line(role, "TX -> ITEM_CMD (1 byte) ...");
        ok = Zigbee_SendItemReliable(ITEM_CMD, &cmd, 1, 0);
        log_line(role, "TX -> ITEM_CMD : %s", ok ? "OK" : "FAIL");
    }

    log_line(role, "----------------------------------------");
}

void on_key_a_click(void) {
    log_main_single_test_once(ROLE_B);
}

void on_key_b_click(void) {
    log_main_single_test_once(ROLE_A);
}

void on_key_c_click(void) {
  
}

void on_key_d_click(void) {
  
}

void btn_callback(void *btn_ptr) {
    alarm_log1();
    struct Button* btn = (struct Button*)btn_ptr;
    uint8_t id = btn->button_id;   // ✅ 直接取值
    switch (id)
    {
        case KEY_A: log_main("key a pressed");  on_key_a_click(); break;
        case KEY_B: log_main("key b pressed");  on_key_b_click(); break;
        case KEY_C: log_main("key c pressed");  on_key_c_click(); break;
        case KEY_D: log_main("key d pressed");  on_key_d_click(); break;
        default:
            log_main("Unknown key id: %d", id);
            alarm_fail();
            break;
    }
}

void setup()
{
	// CPP库百科荣创官方维护
	CoreLED.Initialization();           //核心板led灯初始化
	CoreBeep.Initialization();		    //核心板蜂鸣器初始化
	ExtSRAMInterface.Initialization();  //底层通讯初始化
	LED.Initialization();               // 任务板LED初始化
	BH1750.Initialization();            // 光强
	BEEP.Initialization();              // 任务板蜂鸣器
	Infrare.Initialization();           // 红外
	DCMotor.Initialization();           //电机
	BKRC_Voice.Initialization();        //小创初始化
	// C库用户维护
    Ultrasonic_Init();
	Log_Init(115200);//串口初始化
	K210_SerialInit();
	Servo_SetAngle(SERVO_DEFAULT_ANGLE);
    muti_button_init(btn_callback);
	// 是否使用陀螺仪
#ifdef USE_GYRO
	LSM6DSV16X_Init();
	LSM6DSV16X_RezeroYaw_Fresh();
	delay(2000); // 强行静止2秒
#endif 
    alarm_log2();
}

void loop()
{
    static uint32_t last = 0;
    static uint8_t led = 0;
    uint32_t now = millis();
    if (now - last >= 100) {
        led ^= 1;
        CoreLED.TurnOnOff(led);
        last = now;
    }
    button_ticks();
    delay(5);
}          
