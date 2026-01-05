#ifndef K210_APP_H__
#define K210_APP_H__

#include "zigbee_driver.hpp"

#define QRCODE_SLOTS     3
#define QRCODE_SLOT_SIZE 64

/* ================= 舵机角度宏定义 ================= */
#define SERVO_TRAFFIC_IDENTIFY_ANGLE   15     // 交通灯识别角度
#define SERVO_QRCODE_IDENTIFY_ANGLE    -10
#define SERVO_DEFAULT_ANGLE           (-45) // 舵机回正角度
#define SERVO_SCAN_MIN                (-15)
#define SERVO_SCAN_MAX                (-2)
#define SERVO_SCAN_STEP               (1)


uint8_t identifyTraffic_New(TrafficID id);

void identifyQrCode_New(uint8_t sheet);
void Servo_SetAngle(int angle);

#endif 