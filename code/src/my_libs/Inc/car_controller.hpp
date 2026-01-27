// car_controller.hpp
#ifndef CAR_CONTROLLER_HPP__
#define CAR_CONTROLLER_HPP__

#include <stdint.h>

/* ================== 类型定义 ================== */
typedef enum
{
    TURN_LEFT = 0,
    TURN_RIGHT
} TurnDirection;

/* ================== 对外 API ================== */
void Car_MoveForward(uint8_t speed, uint16_t distance);
void Car_MoveBackward(uint8_t speed, uint16_t distance);

// angle_deg: +90 左转90度，-90 右转90度
void Car_Turn(int16_t angle_deg);

/* K210 摄像头循迹 */
void Car_TrackToCross(uint8_t speed);
void Car_TrackForward(uint8_t speed, uint16_t distance);

/* 15路循迹板（SRAM） */
void Car_TrackToCrossTrackingBoard(uint8_t speed);
void Car_TrackForwardTrackingBoard(uint8_t speed, uint16_t distance, uint8_t kp = 1u);

/* 陀螺仪 */
void Car_Turn_Gryo(float target_yaw_deg); // angle: 绝对目标 yaw, [-180,180]
void Car_MoveForward_Gyro(uint16_t speed, uint16_t distance, float target_yaw);
void Car_MoveBackward_Gyro(uint16_t speed, uint16_t distance, float target_yaw);

/* 超声波闭环距离 */
void Car_MoveToTarget(float targetDistance_cm);

/* 复合动作 */
void Car_BackIntoGarage_Cam(void);
void Car_BackIntoGarage_Gyro(float angle);
void Car_BackIntoGarage_TrackingBoard(int n);
void Car_PassSpecialTerrain(void);

#endif
