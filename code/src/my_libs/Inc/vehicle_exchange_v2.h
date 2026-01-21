#ifndef VEHICLE_EXCHANGE_V2_H__
#define VEHICLE_EXCHANGE_V2_H__

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "zigbee_frame_io.h"

/* ===================== 类型规划：offset 编进 type ===================== */
#define TYPE_DATA_BASE   0x80   // 0x80~0xBF : offset 0~63
#define TYPE_ACK_BASE    0xC0   // 0xC0~0xFF : offset 0~63
#define MAX_OFFSET       64

/* ===================== 你自己定义 item_id（示例） ===================== */
typedef enum {
    ITEM_CAR_PLATE        = 0x01, // len 6
    ITEM_DATE_YYYYMMDD    = 0x02, // len 3
    ITEM_TIME_HHMMSS      = 0x03, // len 3

    ITEM_GARAGE_FINAL_FLOOR = 0x10, // len 1
    ITEM_TERRAIN_STATUS     = 0x11, // len 1
    ITEM_TERRAIN_POSITION   = 0x12, // len 1
    ITEM_VOICE_BROADCAST    = 0x13, // len 1

    ITEM_BEACON_CODE        = 0x20, // len 6
    ITEM_TFT_DATA           = 0x21, // len 6
    ITEM_WIRELESS_CHARGE    = 0x22, // len 6

    ITEM_STREET_LIGHT       = 0x30, // len 2 (init,final)
    ITEM_TRAFFIC_LIGHT_ABC  = 0x31, // len 3
    ITEM_3D_DISPLAY         = 0x32, // len 12

    ITEM_BUS_STOP_TEMP      = 0x40, // len 2
    ITEM_GARAGE_AB_FLOOR    = 0x41, // len 2
    ITEM_DISTANCE           = 0x42, // len 3
    ITEM_GARAGE_INIT_COORD  = 0x43, // len 3
    ITEM_RANDOM_ROUTE       = 0x44, // len 12

    ITEM_RFID               = 0x50, // len 16
    ITEM_CMD                = 0x51, // len 1
} ItemId;

/* 发送一个 item（buf长度由 len 指定），可靠送达（停等ACK） */
bool Zigbee_SendItemReliable(uint8_t item_id,
                             const uint8_t *buf,
                             uint8_t len,
                             uint32_t timeout_ms);

/* 接收一个 item（收到完整 item 才返回 true） */
bool Zigbee_WaitItemReliable(uint8_t *out_item_id,
                             uint8_t *out_buf,
                             uint8_t *inout_len,   // 输入：buf容量；输出：实际长度
                             uint32_t timeout_ms);

#endif
