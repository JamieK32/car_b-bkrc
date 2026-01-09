#ifndef VEHICLE_EXCHANGE_H__
#define VEHICLE_EXCHANGE_H__

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "zigbee_frame_io.h"

typedef enum {
    CMD_START1 = 0x12,
    CMD_START2,
    CMD_START3,
    CMD_START4,
} ControlCmd;

typedef enum
{
    ENUM_CAR_PLATE_FRONT_3 = 0x09,     // 车牌前三位（三位数据）
    ENUM_CAR_PLATE_BACK_3,          // 车牌后三位（三位数据）

    ENUM_DATE_YYYYMMDD,             // 年月日（三位数据）
    ENUM_TIME_HHMMSS,               // 时分秒（三位数据）

    ENUM_GARAGE_FINAL_FLOOR,        // 车库最终层数（一位数据）
    ENUM_TERRAIN_STATUS,            // 地形状态（一位数据）

    ENUM_VOICE_BROADCAST,            // 语音播报（一位数据）

    ENUM_BEACON_CODE_LOW_3,          // 烽火台激活码低3位（三位数据）
    ENUM_BEACON_CODE_HIGH_3,         // 烽火台激活码高3位（三位数据）

    ENUM_TFT_DATA_LOW_3,             // TFT数据低3位（三位数据）
    ENUM_TFT_DATA_HIGH_3,            // TFT数据高3位（三位数据）

    ENUM_STREET_LIGHT_INIT_LEVEL,    // 路灯初始档位（一位数据）
    ENUM_STREET_LIGHT_FINAL_LEVEL,   // 路灯最终档位（一位数据）

    ENUM_TERRAIN_POSITION,           // 地形的位置（一位数据）

    ENUM_TRAFFIC_LIGHT_ABC_COLOR,    // 交通灯ABC颜色（三位数据）

    ENUM_3D_DISPLAY_DATA1,           // 立体显示数据1（三位数据）
    ENUM_3D_DISPLAY_DATA2,           // 立体显示数据2（三位数据）
    ENUM_3D_DISPLAY_DATA3,           // 立体显示数据3（三位数据）
    ENUM_3D_DISPLAY_DATA4,           // 立体显示数据4（三位数据）

    ENUM_WIRELESS_CHARGE_LOW_3,      // 无线充电激活码低3位（三位数据）
    ENUM_WIRELESS_CHARGE_HIGH_3,     // 无线充电激活码高3位（三位数据）

    ENUM_BUS_STOP_TEMPERATURE,       // 公交车站天气温度（两位数据）

    ENUM_GARAGE_AB_FLOOR,            // 获取车库AB的层数（两位数据）

    ENUM_DISTANCE_MEASURE,           // 测距（三位数据）

    ENUM_GARAGE_INIT_COORD,          // 初始车库坐标（三位数据）

    ENUM_RANDOM_ROUTE_POINT1,        // 随机路线点位1（三位数据）
    ENUM_RANDOM_ROUTE_POINT2,        // 随机路线点位2（三位数据）
    ENUM_RANDOM_ROUTE_POINT3,        // 随机路线点位3（三位数据）
    ENUM_RANDOM_ROUTE_POINT4,        // 随机路线点位4（三位数据）

    ENUM_RFID_DATA1,
    ENUM_RFID_DATA2,
    ENUM_RFID_DATA3,
    ENUM_RFID_DATA4,
    ENUM_RFID_DATA5,
    ENUM_RFID_DATA6,

    ENUM_DATA_MAX                    // 枚举结束标志
} CarDataEnum;


typedef struct
{
    uint8_t date[3];   // 年月日
    uint8_t time[3];   // 时分秒
} DateTime_t;

typedef struct
{
    uint8_t init_level;
    uint8_t final_level;
} StreetLight_t;

typedef struct
{
    uint8_t car_plate[6];            // 车牌号（6位）

    DateTime_t datetime;             // 年月日 + 时分秒

    uint8_t garage_final_floor;      // 车库最终层数
    uint8_t terrain_status;          // 地形状态
    uint8_t terrain_position;        // 地形位置

    uint8_t voice_broadcast;         // 语音播报

    uint8_t beacon_code[6];          // 烽火台激活码（低3 + 高3）
    uint8_t tft_data[6];             // TFT 数据（低3 + 高3）
    uint8_t wireless_charge_code[6]; // 无线充电激活码（低3 + 高3）

    StreetLight_t street_light;      // 路灯（初始 + 最终）

    uint8_t traffic_light_abc[3];    // 交通灯ABC颜色

    uint8_t display_3d[12];          // 立体显示数据 1~4

    uint8_t bus_stop_temp[2];        // 公交站天气温度
    uint8_t garage_ab_floor[2];      // 车库 AB 层数

    uint8_t distance[3];             // 测距
    uint8_t garage_init_coord[3];    // 初始车库坐标

    uint8_t random_route[12];        // 随机路线点位 1~4
    
    uint8_t rfid[16];                // 读卡数据
} ProtocolData_t;


/* ===================== CMD ===================== */
void Send_Start_Cmd(void);
bool Wait_Start_Cmd(uint32_t timeout_ms);

/* ===================== DATA ===================== */
/* ✅ SendData：只发送非0字段；发送后自动清零（避免重复发） */
bool SendData(ProtocolData_t *inout);

/* ✅ WaitData：接收期间不断填充 out；若收到至少1帧数据则返回 true */
bool WaitData(ProtocolData_t *out, uint32_t timeout_ms);

#endif /* VEHICLE_EXCHANGE_H__ */
