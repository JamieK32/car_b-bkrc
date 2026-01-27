// car_controller_gyro_ultra.cpp
#include "car_controller.hpp"

#include "DCMotor.h"
#include "lsm6dsv16x.h"
#include "ultrasonic.hpp"

#include <Arduino.h>
#include <math.h>
#include <stdint.h>

/* ================== 陀螺仪直行常量 ================== */
static const uint16_t GYRO_KP = 15;
static const float MAX_CORR  = 100.0f;
static const float MIN_SPD   = -100.0f;  // 不想反转就设 0
static const float MAX_SPD   =  100.0f;

static euler_angles_t euler_angle;

/* ================== 陀螺仪转向常量 ================== */
static const float   KP_TURN    = 5.0f;
static const int16_t MIN_TURN   = 40;
static const int16_t MAX_TURN   = 90;
static const float   STOP_TH    = 2.0f;
static const uint8_t STABLE_N   = 8;
static const uint32_t TIMEOUT   = 5000;

/* ================== 超声波闭环常量 ================== */
static const uint8_t MAX_SPEED = 40;
static const uint8_t MIN_SPEED = 15;
static const float   TOLERANCE = 0.5f;
static const float   KP        = 4.0f;
static const float   KI        = 0.1f;
static const float   INTEGRAL_LIMIT = 10.0f;

/* ================== 超声波摆正（左右试探）常量 ================== */
static const uint8_t  ULTRA_ALIGN_TURN_SPD   = 75;   // 原地试探转向速度（别太大）
static const uint16_t ULTRA_PROBE_MS         = 130;   // 试探一次转动时长（越大角度越大）
static const uint16_t ULTRA_SETTLE_MS        = 130;   // 停稳等待（很关键）
static const uint8_t  ULTRA_SAMPLES          = 5;    // 中值滤波次数
static const float    ULTRA_DEADBAND_CM      = 0.0f; // 左右距离差小于它就算摆正
static const uint16_t ULTRA_MAX_ALIGN_MS     = 220;  // 单次修正最大转动时长
static const uint32_t ULTRA_ALIGN_TIMEOUT_MS = 10000; // 总超时
static const float    ULTRA_ALIGN_K_MS_PER_CM = 60.0f; // |dL-dR| -> 修正时长系数

/* ================== 工具函数 ================== */
static inline float clampf(float v, float lo, float hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static inline float wrap180(float a)
{
    while (a > 180.0f) a -= 360.0f;
    while (a < -180.0f) a += 360.0f;
    return a;
}

static inline float calculate_angle_error(float target, float current)
{
    float error = target - current;

    if (error > 180.0f)      error -= 360.0f;
    else if (error < -180.0f) error += 360.0f;

    return error;
}

/* ================== 对外：陀螺仪控制 ================== */
void Car_Turn_Gryo(float angle)   // angle: 绝对目标 yaw, [-180,180]
{
    float target_yaw = wrap180(angle);
    uint32_t start = millis();
    uint8_t stable = 0;

    while (millis() - start < TIMEOUT)
    {
        Read_LSM6DSV16X(&euler_angle);

        float err  = calculate_angle_error(target_yaw, euler_angle.yaw);
        float aerr = fabsf(err);

        if (aerr < STOP_TH)
        {
            if (++stable >= STABLE_N) break;
        }
        else
        {
            stable = 0;
        }

        float v = clampf(aerr * KP_TURN, (float)MIN_TURN, (float)MAX_TURN);

        int16_t left, right;
        if (err > 0) { left = -(int16_t)v; right = (int16_t)v; }
        else         { left = (int16_t)v;  right = -(int16_t)v; }

        DCMotor.SpeedCtr(left, right);
    }

    DCMotor.SpeedCtr(0, 0);
}

void Car_MoveForward_Gyro(uint16_t speed, uint16_t distance, float target_yaw)
{
    DCMotor.Roadway_mp_sync();

    while (DCMotor.Roadway_mp_Get() < distance)
    {
        Read_LSM6DSV16X(&euler_angle);

        float error  = calculate_angle_error(target_yaw, euler_angle.yaw);
        float output = clampf(error * (float)GYRO_KP, -MAX_CORR, MAX_CORR);

        float left  = (float)speed - output;
        float right = (float)speed + output;

        left  = clampf(left,  MIN_SPD, MAX_SPD);
        right = clampf(right, MIN_SPD, MAX_SPD);

        DCMotor.SpeedCtr(left, right);
    }

    DCMotor.SpeedCtr(0, 0);
}

void Car_MoveBackward_Gyro(uint16_t speed, uint16_t distance, float target_yaw)
{
    DCMotor.Roadway_mp_sync();

    while (DCMotor.Roadway_mp_Get() < distance)
    {
        Read_LSM6DSV16X(&euler_angle);

        float error  = calculate_angle_error(target_yaw, euler_angle.yaw);
        float output = clampf(error * (float)GYRO_KP, -MAX_CORR, MAX_CORR);

        float left  = (float)speed + output;
        float right = (float)speed - output;

        left  = clampf(left,  MIN_SPD, MAX_SPD);
        right = clampf(right, MIN_SPD, MAX_SPD);

        DCMotor.SpeedCtr(-left, -right);
    }

    DCMotor.SpeedCtr(0, 0);
}

/* ================== 对外：超声波距离闭环 ================== */
void Car_MoveToTarget(float targetDistance_cm)
{
    float integral = 0.0f;

    while (true)
    {
        float currentDistance = Ultrasonic_GetDistance();
        float error = currentDistance - targetDistance_cm;

        if (fabsf(error) <= TOLERANCE)
        {
            DCMotor.Stop();
            break;
        }

        integral += error;
        integral = clampf(integral, -INTEGRAL_LIMIT, INTEGRAL_LIMIT);

        float output = (KP * error) + (KI * integral);

        float mag = fabsf(output);
        mag = clampf(mag, (float)MIN_SPEED, (float)MAX_SPEED);
        uint8_t moveSpeed = (uint8_t)mag;

        if (error > 0)
        {
            DCMotor.SpeedCtr(moveSpeed, moveSpeed);
        }
        else
        {
            DCMotor.SpeedCtr(-(int16_t)moveSpeed, -(int16_t)moveSpeed);
        }

        delay(30);
    }
}

static inline float Ultrasonic_ReadStable_cm()
{
    // 用中值更稳；如果你不想用 median，也可以换成 Ultrasonic_GetDistance()
    unsigned int d = Ultrasonic_GetDistanceMedian(ULTRA_SAMPLES);
    return (float)d;
}

/* ================== 对外：超声波摆正（左右试探） ================== */
void Car_SquareByUltrasonicProbe()
{
    uint32_t start = millis();

    while (millis() - start < ULTRA_ALIGN_TIMEOUT_MS)
    {
        // 先停稳再开始一次试探
        DCMotor.SpeedCtr(0, 0);
        delay(ULTRA_SETTLE_MS);

        // ---- 左试探：左转一点 -> 测距 dL ----
        DCMotor.SpeedCtr(-(int16_t)ULTRA_ALIGN_TURN_SPD, (int16_t)ULTRA_ALIGN_TURN_SPD);
        delay(ULTRA_PROBE_MS);
        DCMotor.SpeedCtr(0, 0);
        delay(ULTRA_SETTLE_MS);
        float dL = Ultrasonic_ReadStable_cm();

        // ---- 右试探：从左侧位置直接右转 2 倍时间 -> 到右侧 -> 测距 dR ----
        DCMotor.SpeedCtr((int16_t)ULTRA_ALIGN_TURN_SPD, -(int16_t)ULTRA_ALIGN_TURN_SPD);
        delay(ULTRA_PROBE_MS * 2);
        DCMotor.SpeedCtr(0, 0);
        delay(ULTRA_SETTLE_MS);
        float dR = Ultrasonic_ReadStable_cm();

        // ---- 回到中心：左转 1 倍时间回中 ----
        DCMotor.SpeedCtr(-(int16_t)ULTRA_ALIGN_TURN_SPD, (int16_t)ULTRA_ALIGN_TURN_SPD);
        delay(ULTRA_PROBE_MS);
        DCMotor.SpeedCtr(0, 0);
        delay(ULTRA_SETTLE_MS);

        // 计算左右差值：dL - dR
        float err = dL - dR;

        // 若左右差很小，认为已摆正
        if (fabsf(err) <= ULTRA_DEADBAND_CM)
        {
            DCMotor.SpeedCtr(0, 0);
            return;
        }

        // 根据误差决定往哪边“修正”
        // 直觉：哪边距离更小，说明那边更对着墙（更正），就朝那边转一点
        // err < 0 => dL < dR => 左边更近 => 往左修正
        // err > 0 => dR < dL => 右边更近 => 往右修正
        float ms_f = fabsf(err) * ULTRA_ALIGN_K_MS_PER_CM;
        uint16_t fix_ms = (uint16_t)clampf(ms_f, 30.0f, (float)ULTRA_MAX_ALIGN_MS);

        if (err < 0)
        {
            // 左修正
            DCMotor.SpeedCtr(-(int16_t)ULTRA_ALIGN_TURN_SPD, (int16_t)ULTRA_ALIGN_TURN_SPD);
            delay(fix_ms);
        }
        else
        {
            // 右修正
            DCMotor.SpeedCtr((int16_t)ULTRA_ALIGN_TURN_SPD, -(int16_t)ULTRA_ALIGN_TURN_SPD);
            delay(fix_ms);
        }

        DCMotor.SpeedCtr(0, 0);
        delay(ULTRA_SETTLE_MS);
    }

    // 超时兜底
    DCMotor.SpeedCtr(0, 0);
}
