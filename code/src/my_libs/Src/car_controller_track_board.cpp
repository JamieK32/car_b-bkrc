// car_controller_track_board.cpp
#include "car_controller.hpp"

#include "DCMotor.h"
#include <ExtSRAMInterface.h>

#include <Arduino.h>
#include <math.h>
#include <stdint.h>

/* ================== 参数 ================== */
static const uint16_t MILEAGE_EXIT   = 1800;
static const uint16_t MILEAGE_MIN    = 600;
static const uint16_t FINAL_DISTANCE = 355;

#define OFFSET_STOP_TH   2.0f    // 偏移阈值（单位：探头索引）
#define SPEED_MIN_ABS    60.0f   // 最小幅度限制

static const int16_t MAX_TURN = 90;

/* ================== 工具函数 ================== */
static inline float clampf(float v, float lo, float hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static inline float clamp_min_abs(float x, float min_abs)
{
    if (x > 0.0f && x < min_abs)  return  min_abs;
    if (x < 0.0f && x > -min_abs) return -min_abs;
    return x;
}

/**
 * @brief 读取 15 路循迹板（与原字符串实现“完全等价”，但用位运算更快）
 * 你原逻辑：偶数位取 low8 的 bit0..7，奇数位取 high7 的 bit0..6，然后反转字符串再转二进制
 * 等价结果：最终整数的 bit i 就是“原循环第 i 位的采样值”
 */
static inline uint16_t readTracking15(void)
{
    uint8_t low8  = (uint8_t)ExtSRAMInterface.ExMem_Read(0x6000);
    uint8_t high8 = (uint8_t)ExtSRAMInterface.ExMem_Read(0x6001);
    uint8_t high7 = (uint8_t)(high8 & 0x7Fu);

    uint16_t v = 0;
    for (int i = 0; i < 15; i++)
    {
        uint8_t bit;
        if ((i & 1) == 0) bit = (low8  >> (i / 2)) & 1u;
        else              bit = (high7 >> (i / 2)) & 1u;

        v |= ((uint16_t)bit << i);
    }
    return v; // 0..0x7FFF
}

static inline float getLookupOffset15(uint16_t lineMask)
{
    // 0=触发 → 翻转成 1=有效（只处理 15 位）
    uint16_t active = (uint16_t)((~lineMask) & 0x7FFFu);

    int sum = 0;
    int cnt = 0;

    for (int i = 0; i < 15; i++)
    {
        if (active & (1u << i))
        {
            sum += i;
            cnt++;
        }
    }

    if (cnt == 0) return 0.0f;     // 全1（0x7FFF）=> active=0

    float centroid = (float)sum / (float)cnt; // 0..14
    return centroid - 7.0f;                   // 15 路中心是 7
}

static inline bool isAtHighSpeedCrossroad16(uint16_t lineMask)
{
    uint16_t active = (uint16_t)((~lineMask) & 0x7FFFu);

    bool left  = (active & 0b111000000000000u) != 0; // bits 12,13,14
    bool right = (active & 0b000000000000111u) != 0; // bits 0,1,2

    return left && right;
}

/* ================== 对外：15路循迹板循迹 ================== */
void Car_TrackToCrossTrackingBoard(uint8_t speed)
{
    DCMotor.Roadway_mp_sync();

    while (true)
    {
        uint16_t lineMask = readTracking15();
        uint16_t mileage  = DCMotor.Roadway_mp_Get();

        if ((mileage >= MILEAGE_MIN && isAtHighSpeedCrossroad16(lineMask)) ||
            (mileage >= MILEAGE_EXIT))
        {
            break;
        }

        float offset = getLookupOffset15(lineMask);

        if (fabsf(offset) > OFFSET_STOP_TH)
        {
            float speed_turn = offset * 35.0f;
            speed_turn = clampf(speed_turn, (float)-MAX_TURN, (float)MAX_TURN);
            DCMotor.SpeedCtr(-speed_turn, speed_turn);
        }
        else
        {
            float corr = offset * 20.0f;
            DCMotor.SpeedCtr(speed - corr, speed + corr);
        }
    }

    DCMotor.Stop();
    DCMotor.Car_Go(speed, FINAL_DISTANCE);
}

void Car_TrackForwardTrackingBoard(uint8_t speed, uint16_t distance, uint8_t kp)
{
    if (kp == 0) kp = 1u;

    DCMotor.Roadway_mp_sync();

    while (DCMotor.Roadway_mp_Get() < distance)
    {
        uint16_t lineMask = readTracking15();
        float offset = getLookupOffset15(lineMask);

        if (fabsf(offset) > (OFFSET_STOP_TH - 1.0f))
        {
            float speed_turn = offset * 35.0f / (float)kp;
            speed_turn = clampf(speed_turn, (float)-MAX_TURN, (float)MAX_TURN);

            float left  = -speed_turn;
            float right =  speed_turn;

            left  = clamp_min_abs(left,  SPEED_MIN_ABS);
            right = clamp_min_abs(right, SPEED_MIN_ABS);

            DCMotor.SpeedCtr(left, right);
        }
        else
        {
            float corr = offset * 35.0f / (float)kp;

            float left  = (float)speed - corr;
            float right = (float)speed + corr;

            left  = clamp_min_abs(left,  SPEED_MIN_ABS);
            right = clamp_min_abs(right, SPEED_MIN_ABS);

            DCMotor.SpeedCtr(left, right);
        }
    }

    DCMotor.Stop();
}
