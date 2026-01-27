// car_controller_common.cpp
#include "car_controller.hpp"

#include "DCMotor.h"
#include "LED.h"

#include <Arduino.h>

/* ================== 转向宏定义 ================== */
#define BASE_TURN_SPEED   90
#define TURN_90_DEG       685

/* ================== 底层功能（解耦） ================== */
static inline void TurnSignal_On(TurnDirection dir);
static inline void TurnSignal_Off(TurnDirection dir);
static inline void Motor_TurnAngle(TurnDirection dir, uint8_t speed, uint16_t distance);

/**
 * @brief 小车前进/后退（编码器距离）
 */
void Car_MoveForward(uint8_t speed, uint16_t distance)
{
    DCMotor.Car_Go(speed, distance);
}

void Car_MoveBackward(uint8_t speed, uint16_t distance)
{
    DCMotor.Car_Back(speed, distance);
}

/**
 * @brief 小车转向（编码器角度映射）
 */
void Car_Turn(int16_t angle_deg)
{
    TurnDirection dir = (angle_deg >= 0) ? TURN_LEFT : TURN_RIGHT;
    uint16_t deg = (angle_deg >= 0) ? (uint16_t)angle_deg : (uint16_t)(-angle_deg);

    uint16_t mp = (uint16_t)((deg * (uint32_t)TURN_90_DEG) / 90);

    TurnSignal_On(dir);
    Motor_TurnAngle(dir, BASE_TURN_SPEED, mp);
    TurnSignal_Off(dir);
}

/* ================== 复合动作（依赖其它模块 API） ================== */
void Car_BackIntoGarage_Cam(void)
{
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

void Car_BackIntoGarage_TrackingBoard(int n)
{
    Car_Turn(155);
    Car_MoveForward(80, 300);
    for (int i = 0; i < n; i++)
    {
        Car_TrackForwardTrackingBoard(75, 300, (uint8_t)(i + 1));
        DCMotor.Car_Back(75, 300);
    }
    DCMotor.Car_Back(75, 940);
}

void Car_PassSpecialTerrain(void)
{
    Car_MoveBackward(80, 300);
    Car_TrackForwardTrackingBoard(80, 400);
    Car_MoveForward(80, 1750);
}

/* ================== 底层功能（解耦） ================== */
static inline void TurnSignal_On(TurnDirection dir)
{
    if (dir == TURN_LEFT) LED.LeftTurnOn();
    else                 LED.RightTurnOn();
}

static inline void TurnSignal_Off(TurnDirection dir)
{
    if (dir == TURN_LEFT) LED.LeftTurnOff();
    else                 LED.RightTurnOff();
}

static inline void Motor_TurnAngle(TurnDirection dir, uint8_t speed, uint16_t distance)
{
    if (dir == TURN_LEFT) DCMotor.TurnLeftAngle(speed, distance);
    else                 DCMotor.TurnRightAngle(speed, distance);
}
