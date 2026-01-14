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
#include "ultrasonic.hpp"

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
static const float OFFSET_MULTIPLIER = 43.0f;	   // 偏移量倍数
static const uint16_t FINAL_DISTANCE = 750;	       // 最终前进距离

/* ================== 陀螺仪直行常量 ================== */
static const uint16_t GYRO_KP = 15;
const float MAX_CORR = 100;           // 修正上限（按你PWM范围调）
const float MIN_SPD  = -100;           // 最小速度（不想反转就设 0）
const float MAX_SPD  = 100;          // 最大速度（按你SpeedCtr范围调）
static euler_angles_t euler_angle;

/* ================== 陀螺仪转向常量 ================== */
const float KP_TURN     = 5.0f;
const int16_t MIN_TURN  = 40;
const int16_t MAX_TURN  = 90;
const float STOP_TH     = 2.0f;
const uint8_t STABLE_N  = 8;
const uint32_t TIMEOUT  = 5000;

/* ================== 超声波闭环常量 ================== */
const uint8_t MAX_SPEED = 40;      // 限制最大速度，防止速度过快
const uint8_t MIN_SPEED = 15;       // 启动死区速度（电机能转动的最小速度）
const float TOLERANCE = 0.5;        // 容差范围（cm），在 17.5~18.5 之间停止运行，防止抖动
const float KP = 4;
const float KI = 0.1;
const float INTEGRAL_LIMIT = 10;

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

void Car_BackIntoGarage_Gyro(float angle)
{
    Car_Turn_Gryo(angle);
    Car_TrackForward(40, 700);
    DCMotor.Car_Back(40, 700);
    Car_TrackForward(40, 500);
    DCMotor.Car_Back(40, 1550);
}



void Car_MoveToTarget(float targetDistance) {
    float integral = 0; // 1. 定义积分变量
    
    // 如果你在头文件中没定义这些，可以在这里补充
    // const float KI = 0.5; // 根据实际效果调试，通常比 KP 小很多
    // const float INTEGRAL_LIMIT = 50; // 积分限幅，防止过度累加

    while (true) {
        // 1. 获取当前距离
        float currentDistance = Ultrasonic_GetDistance();
        
        // 2. 计算误差
        float error = currentDistance - targetDistance;

        // 3. 退出条件
        if (abs(error) <= TOLERANCE) {
            DCMotor.Stop();  
            break; 
        }

        // 4. 积分累加 (KI * error)
        // 注意：只有当误差在一定范围内（或者一直存在）时累加
        integral += error;

        // 5. 积分限幅 (重要：防止积分饱和)
        // 如果不限制，当小车被挡住时，integral 会变无限大，移开障碍物后小车会突然猛冲
        integral = constrain(integral, -INTEGRAL_LIMIT, INTEGRAL_LIMIT);

        // 6. 计算 PI 输出
        // Output = P项 + I项
        float output = (KP * error) + (KI * integral);
        
        uint8_t moveSpeed = (uint8_t)constrain(abs(output), MIN_SPEED, MAX_SPEED);

        // 7. 执行动作
        if (error > 0) {
            // 距离太远，向前
            DCMotor.SpeedCtr(moveSpeed, moveSpeed);
        } else {
            // 距离太近，向后
            DCMotor.SpeedCtr(-moveSpeed, -moveSpeed);
        }

        // 8. 采样周期控制
        // PID 算法需要固定的时间间隔来保证积分和微分的物理意义
        delay(30); 
    }
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
