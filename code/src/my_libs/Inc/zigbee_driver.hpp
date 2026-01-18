#ifndef ZIGBEE_DRIVER_HPP__
#define ZIGBEE_DRIVER_HPP__

#pragma once
#include <stdint.h>

#define ZIGBEE_TX_ADDR  0x6008
#define ZIGBEE_RX_ADDR  0x6100
#define ZIGBEE_FRAME_LEN 8

/* 交通灯编号 */
typedef enum {
    TRAFFIC_A = 0,
    TRAFFIC_B,
    TRAFFIC_C,
    TRAFFIC_D
} TrafficID;

/* 交通灯颜色 */
typedef enum {
    LIGHT_RED = 0,
    LIGHT_GREEN = 1,
    LIGHT_YELLOW = 2
} TrafficColor;

typedef enum {
    GARAGE_TYPE_A = 0,
    GARAGE_TYPE_B = 1,
} GarageType;

typedef enum {
    LED_TIMER_CONTROL_STOP = 0,
    LED_TIMER_CONTROL_START,
    LED_TIMER_CONTROL_ZERO,
} LedTimerControl;

typedef enum {
    ZB_WEATHER_WINDY   = 0, // 大风
    ZB_WEATHER_CLOUDY  = 1, // 多云
    ZB_WEATHER_SUNNY   = 2, // 晴
    ZB_WEATHER_SNOW    = 3, // 小雪
    ZB_WEATHER_RAIN    = 4, // 小雨
    ZB_WEATHER_OVERCAST= 5  // 阴
} ZigbeeBusWeather;

typedef enum {
    TFT_ID_A = 0x0B,
    TFT_ID_B = 0x08,
} TftId;

typedef enum {
    TFT_TIMER_OFF   = 0x00, // 关
    TFT_TIMER_ON    = 0x01, // 开
    TFT_TIMER_RESET = 0x02  // 重置
} TftTimerAction;

typedef enum {
    TFT_SIGN_STRAIGHT          = 1, // 直行
    TFT_SIGN_LEFT_TURN         = 2, // 左转
    TFT_SIGN_RIGHT_TURN        = 3, // 右转
    TFT_SIGN_U_TURN            = 4, // 掉头
    TFT_SIGN_NO_STRAIGHT       = 5, // 禁止直行
    TFT_SIGN_NO_ENTRY          = 6  // 禁止通行
} TrafficSignType;


typedef struct {
    uint8_t yy, mon, day;
    uint8_t hour, min, sec;
    uint8_t weather;
    int8_t  temp;
} ZigbeeBusInfo;


/* ================== 函数声明 ================== */
void zigbee_send(uint8_t *cmd, uint8_t retry_cnt = 3U, uint16_t interval_ms = 300U);

void Zigbee_Traffic_Open(TrafficID id);
void Zigbee_Traffic_SetColor(TrafficID id, TrafficColor color);

void Zigbee_Garage_Ctrl(GarageType type, int target_floor, bool is_wait = true, unsigned long timeout_ms = 20000);

void Zigbee_Gate_SetState(bool state);
void Zigbee_Gate_Display_LicensePlate(const char* licence_str);

void Zigbee_LED_Display_Hex(uint8_t rank, uint8_t val);
void Zigbee_LED_Display_Number(uint8_t rank, const char* str);
void Zigbee_LED_Display_Distance(uint8_t val);
void Zigbee_LED_TimerControl(LedTimerControl state);

void Zigbee_Bus_FullSync_System(const ZigbeeBusInfo *info);
void Zigbee_Bus_SpeakPreset(uint8_t voice_id);
void Zigbee_Bus_SpeakRandom(void);
bool Zigbee_Bus_Read_Date(uint8_t* yy, uint8_t* mon, uint8_t* day, uint16_t wait_time = 800);
bool Zigbee_Bus_Read_Time(uint8_t* hour, uint8_t* min, uint8_t* sec, uint16_t wait_time = 800);
bool Zigbee_Bus_Read_Env(uint8_t* weather, int8_t* temp, uint16_t wait_time = 800);
bool Zigbee_Bus_Read_All(ZigbeeBusInfo* out, uint16_t wait_time_each = 800);

void Zigbee_TFT_Page_Turn(TftId id, uint8_t mode);
void Zigbee_TFT_Goto_Page(TftId id, uint8_t page);
void Zigbee_TFT_Display_Plate(TftId id, const char* plate);
void Zigbee_TFT_Display_Distance(TftId id, uint16_t dist);
void Zigbee_TFT_Timer_Control(TftId id, TftTimerAction action);
void Zigbee_TFT_Display_Traffic_Sign(TftId id, TrafficSignType sign);
void Zigbee_TFT_Display_Hex(TftId id, uint8_t hex1, uint8_t hex2, uint8_t hex3);

void Zigbee_Wireless_Activate(uint8_t* keys);

void PrintBusInfo(const ZigbeeBusInfo* info);

#endif 
