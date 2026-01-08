#include "DCMotor.h"
#include <ExtSRAMInterface.h>
#include <BH1750.h>
#include <Infrare.h>

#include "Drive.h"
#include "CoreBeep.h"
#include "BEEP.h"
#include <Command.h>
#include "LED.h"
#include "BKRC_Voice.h"


#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <avr/pgmspace.h>

#include "car_controller.hpp"
#include "k210_protocol.hpp"
#include "k210_app.hpp"
#include "log.hpp"
#include "lsm6dsv16x.h"

/* ================== 转向宏定义 ================== */
#define BASE_TURN_SPEED   100
#define TURN_90_DEG       950

/* ================== 循迹常量 ================== */
static const uint16_t MILEAGE_EXIT = 1800;		   // 退出里程阈值
static const uint16_t MILEAGE_MIN = 600;		   // 最小里程要求
static const float OFFSET_MULTIPLIER = 34.0f;	   // 偏移量倍数
static const uint16_t FINAL_DISTANCE = 630;	       // 最终前进距离


static const uint16_t GYRO_KP = 15;
const float MAX_CORR = 100;           // 修正上限（按你PWM范围调）
const float MIN_SPD  = -100;           // 最小速度（不想反转就设 0）
const float MAX_SPD  = 100;          // 最大速度（按你SpeedCtr范围调）
static euler_angles_t euler_angle;

const float KP_TURN     = 5.0f;
const int16_t MIN_TURN  = 60;
const int16_t MAX_TURN  = 90;
const float STOP_TH     = 2.0f;
const uint8_t STABLE_N  = 8;
const uint32_t TIMEOUT  = 5000;


/* ================== 底层功能（解耦） ================== */
static inline void TurnSignal_On(TurnDirection dir);
static inline void TurnSignal_Off(TurnDirection dir);
static inline void Motor_TurnAngle(TurnDirection dir, uint8_t speed, uint16_t distance);
static inline float getLookupOffset(uint8_t lineMask);
static inline uint8_t popcount8(uint8_t x);
static inline bool isAtHighSpeedCrossroad(uint8_t lineMask);
static inline float calculate_angle_error(float target, float current);
static inline float wrap180(float a);
static inline float clampf(float v, float lo, float hi);
static inline float absf(float x);

/**
 * @brief 小车前进
 */
void Car_MoveForward(uint8_t speed, uint16_t distance)
{
    DCMotor.Car_Go(speed, distance);
}

void Car_MoveBackward(uint8_t speed, uint16_t distance)
{
    DCMotor.Car_Back(speed, distance);
}

// angle_deg: +90 左转90度，-90 右转90度
void Car_Turn(int16_t angle_deg)
{
    TurnDirection dir = (angle_deg >= 0) ? TURN_LEFT : TURN_RIGHT;
    uint16_t deg = (angle_deg >= 0) ? angle_deg : -angle_deg;

    uint16_t mp = (uint16_t)((deg * (uint32_t)TURN_90_DEG) / 90);

    TurnSignal_On(dir);
    Motor_TurnAngle(dir, BASE_TURN_SPEED, mp);
    TurnSignal_Off(dir);
}

void Car_TrackToCross(uint8_t speed)
{
    K210_Frame frame;
    DCMotor.Roadway_mp_sync();
    /* 电机 & 舵机初始化 */
    Servo_SetAngle(SERVO_DEFAULT_ANGLE);
    /* 启动循迹 */
    K210_SendCmd(CMD_TRACE_START, 0, 0);
    while (true)
    {
        /* 必须读到有效帧 */
        if (!K210_ReadFrame(&frame, K210_READ_FIXED)) {
            continue;
        }
       //  K210_ClearRxByFrame(&frame);
        uint8_t  lineMask = frame.fixed.data1;
        uint16_t mileage  = DCMotor.Roadway_mp_Get();
        /* === 退出条件 === */
        if (
            /* 情况 1：跑够最小里程后识别到路口 */
            (mileage >= MILEAGE_MIN && isAtHighSpeedCrossroad(lineMask))
            ||
            /* 情况 2：无条件兜底退出 */
            (mileage >= MILEAGE_EXIT)
        ) {
            break;
        }

        /* === 正常循迹控制 === */
        float offset = getLookupOffset(lineMask) * OFFSET_MULTIPLIER;
        DCMotor.SpeedCtr(speed - offset, speed + offset);
    }
    /* === 退出循迹后的统一处理 === */
    DCMotor.Stop();
    /* === 路口后前进固定距离 === */
    DCMotor.Car_Go(speed, FINAL_DISTANCE);
}

void Car_Turn_Gryo(float angle)   // angle: 绝对目标 yaw, [-180,180]
{
    float target_yaw = wrap180(angle);
    uint32_t start = millis();
    uint8_t stable = 0;

    while (millis() - start < TIMEOUT)
    {
        Read_LSM6DSV16X(&euler_angle);

        float err = calculate_angle_error(target_yaw, euler_angle.yaw);
        float aerr = fabsf(err);

        if (aerr < STOP_TH) {
            if (++stable >= STABLE_N) break;
        } else {
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

        float error  = calculate_angle_error(target_yaw, euler_angle.yaw); // 要求[-180,180]
        float output = error * GYRO_KP;
        output = clampf(output, -MAX_CORR, MAX_CORR);

        float left  = (float)speed - output;
        float right = (float)speed + output;

        left  = clampf(left,  MIN_SPD, MAX_SPD);
        right = clampf(right, MIN_SPD, MAX_SPD);

        DCMotor.SpeedCtr(left, right);
    }

    DCMotor.SpeedCtr(0, 0); // 走完停一下更安全
}

void Car_MoveBackward_Gyro(uint16_t speed, uint16_t distance, float target_yaw)
{
    DCMotor.Roadway_mp_sync();
    while (DCMotor.Roadway_mp_Get() < distance)
    {
        Read_LSM6DSV16X(&euler_angle);

        float error  = calculate_angle_error(target_yaw, euler_angle.yaw); // 要求[-180,180]
        float output = error * GYRO_KP;
        output = clampf(output, -MAX_CORR, MAX_CORR);

        float left  = (float)speed + output;
        float right = (float)speed - output;

        left  = clampf(left,  MIN_SPD, MAX_SPD);
        right = clampf(right, MIN_SPD, MAX_SPD);

        DCMotor.SpeedCtr(-left, -right);
    }

    DCMotor.SpeedCtr(0, 0); // 走完停一下更安全
}

/**
 * @brief 循迹一段距离 (对应原 videoLineMP)
 * @param speed 循迹速度
 * @param distance 循迹的最大里程
 * @return 空
 */
void Car_TrackForward(uint8_t speed, uint16_t distance)
{
    K210_Frame frame;
    DCMotor.Roadway_mp_sync();
    K210_SendCmd(CMD_TRACE_START, 0, 0);
    while (DCMotor.Roadway_mp_Get() < distance)
    {
        if (!K210_ReadFrame(&frame, K210_READ_FIXED)) {
            continue;
        }
        uint8_t lineMask = frame.fixed.data1;
        float offset = getLookupOffset(lineMask) * OFFSET_MULTIPLIER;
        int16_t left_motor = speed - offset;
        int16_t right_motor = speed + offset;
        DCMotor.SpeedCtr(left_motor, right_motor);
    }
    
    DCMotor.Stop();
}

void Car_BackIntoGarage_Cam(void) {
    Car_TrackForward(40, 700);
    DCMotor.Car_Back(40, 700);
    Car_TrackForward(40, 500);
    DCMotor.Car_Back(40, 1550);
}

void Car_BackIntoGarage_Gyro(int speed, int distance, float angle)
{
    Car_Turn_Gryo(angle);
    Car_MoveBackward_Gyro(speed, distance, angle);
}

/* ================== 底层功能（解耦） ================== */
/**
 * @brief 打开转向灯
 */
static inline void TurnSignal_On(TurnDirection dir)
{
    if (dir == TURN_LEFT)
        LED.LeftTurnOn();
    else
        LED.RightTurnOn();
}

/**
 * @brief 关闭转向灯
 */
static inline void TurnSignal_Off(TurnDirection dir)
{
    if (dir == TURN_LEFT)
        LED.LeftTurnOff();
    else
        LED.RightTurnOff();
}

/**
 * @brief 电机转向（不关心灯）
 */
static inline void Motor_TurnAngle(TurnDirection dir, uint8_t speed, uint16_t distance)
{
    if (dir == TURN_LEFT)
        DCMotor.TurnLeftAngle(speed, distance);
    else
        DCMotor.TurnRightAngle(speed, distance);
}

/**
 * 查表获取偏移量（通用函数）
 * @param lineMask 线位置掩码
 * @return 偏移量值（未命中返回上一次）
 */
static inline float getLookupOffset(uint8_t lineMask)
{
    const int table_size = sizeof(lookup_table) / sizeof(lookup_table[0]);
    for (int i = 0; i < table_size; i++)
    {
        if (lookup_table[i].input == lineMask)
        {
            return lookup_table[i].output;
        }
    }
    return 0.0f;  // 未命中返回0
}

static inline uint8_t popcount8(uint8_t x)
{
    uint8_t c = 0;
    while (x) {
        c += (x & 1);
        x >>= 1;
    }
    return c;
}

/**
 * 有6个1以上就返回真
 */
static inline bool isAtHighSpeedCrossroad(uint8_t lineMask)
{
    return popcount8(lineMask) >= 6;
}

static inline float calculate_angle_error(float target, float current) {
    float error = target - current;
    
    if (error > 180.0f) {
        error -= 360.0f;
    } else if (error < -180.0f) {
        error += 360.0f;
    }
    
    return error;
}

static inline float wrap180(float a){
    while (a > 180.0f) a -= 360.0f;
    while (a < -180.0f) a += 360.0f;
    return a;
}

static inline float clampf(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static inline float absf(float x) { return (x >= 0) ? x : -x; }
