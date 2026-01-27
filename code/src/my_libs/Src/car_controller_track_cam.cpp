// car_controller_track_cam.cpp
#include "car_controller.hpp"

#include "DCMotor.h"
#include "Drive.h"          // Servo_SetAngle / SERVO_DEFAULT_ANGLE
#include "k210_protocol.hpp"
#include "k210_app.hpp"

#include <Arduino.h>
#include <math.h>

/* ================== 循迹常量（K210 摄像头） ================== */
static const uint16_t MILEAGE_EXIT       = 1800;   // 退出里程阈值
static const uint16_t MILEAGE_MIN        = 600;    // 最小里程要求
static const float    OFFSET_MULTIPLIER  = 35.0f;  // 偏移量倍数
static const uint16_t FINAL_DISTANCE     = 650;    // 最终前进距离

/* ================== 查表结构 + 表 ================== */
typedef struct
{
    uint8_t input;
    float   output;
} lookup_table_t;

static const lookup_table_t lookup_table[] = {
    {0x01, -3.5}, {0x02, -2.5}, {0x04, -1.5}, {0x08, -0.5},
    {0x10,  0.0}, {0x20,  0.5}, {0x40,  1.5}, {0x80,  2.5},

    {0x03, -3.0}, {0x06, -2.0}, {0x0C, -1.0}, {0x18,  0.0},
    {0x30,  1.0}, {0x60,  2.0}, {0xC0,  3.0},

    {0x07, -2.5}, {0x0E, -1.5}, {0x1C, -0.3}, {0x38,  1.5},
    {0x70,  1.5}, {0xE0,  2.5}, {0x0F, -2.0}, {0x1E, -1.5},
    {0x3C,  0.0}, {0x78,  1.5}, {0xF0,  2.0},

    {0x1F, -1.5}, {0x3E, -0.5}, {0x7C,  0.0}, {0xF8,  0.5},

    {0xFE, -3.5}, {0xFD, -2.5}, {0xFB, -1.5}, {0xF7, -0.5},
    {0xEF,  0.0}, {0xDF,  0.5}, {0xBF,  1.5}, {0x7F,  2.5},

    {0xFC, -3.0}, {0xF9, -2.0}, {0xF3, -1.0}, {0xE7,  0.0},
    {0xCF,  1.0}, {0x9F,  2.0}, {0x3F,  3.0},

    {0xDF, -3}, {0xFF, -3}, {0xEF, -3}, {0xBF, 3.0}, {0xFB, 3.0},
};

/* ================== 内部小工具 ================== */
static inline uint8_t popcount8(uint8_t x)
{
    uint8_t c = 0;
    while (x) { c += (x & 1u); x >>= 1; }
    return c;
}

// 有6个1以上就返回真
static inline bool isAtHighSpeedCrossroad(uint8_t lineMask)
{
    return (((lineMask) & 1) && ((lineMask >> 7) & 1)) || (((lineMask >> 2) & 1) && ((lineMask >> 6) & 1)) || popcount8(lineMask) >= 5;
}

static inline float getLookupOffset(uint8_t lineMask)
{
    const int table_size = (int)(sizeof(lookup_table) / sizeof(lookup_table[0]));
    for (int i = 0; i < table_size; i++)
    {
        if (lookup_table[i].input == lineMask) return lookup_table[i].output;
    }
    return 0.0f;
}

/* ================== 对外：K210 摄像头循迹 ================== */
void Car_TrackToCross(uint8_t speed)
{
    K210_Frame frame;
    DCMotor.Roadway_mp_sync();

    Servo_SetAngle(SERVO_DEFAULT_ANGLE);

    K210_SendCmd(CMD_TRACE_START, 0, 0);

    while (true)
    {
        if (!K210_ReadFrame(&frame, K210_READ_FIXED)) continue;

        uint8_t  lineMask = frame.fixed.data1;
        uint16_t mileage  = DCMotor.Roadway_mp_Get();

        if ((mileage >= MILEAGE_MIN && isAtHighSpeedCrossroad(lineMask)) ||
            (mileage >= MILEAGE_EXIT))
        {
            break;
        }

        float offset = getLookupOffset(lineMask) * OFFSET_MULTIPLIER;
        DCMotor.SpeedCtr(speed - offset, speed + offset);
    }

    DCMotor.Stop();

    DCMotor.Car_Go(speed, FINAL_DISTANCE);
}

void Car_TrackForward(uint8_t speed, uint16_t distance)
{
    K210_Frame frame;
    DCMotor.Roadway_mp_sync();

    K210_SendCmd(CMD_TRACE_START, 0, 0);

    while (DCMotor.Roadway_mp_Get() < distance)
    {
        if (!K210_ReadFrame(&frame, K210_READ_FIXED)) continue;

        uint8_t lineMask = frame.fixed.data1;
        float offset = getLookupOffset(lineMask) * OFFSET_MULTIPLIER;

        int16_t left_motor  = (int16_t)((float)speed - offset);
        int16_t right_motor = (int16_t)((float)speed + offset);

        DCMotor.SpeedCtr(left_motor, right_motor);
    }

    DCMotor.Stop();
}
