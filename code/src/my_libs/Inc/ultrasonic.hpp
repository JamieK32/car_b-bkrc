#ifndef ULTRASONIC_HPP
#define ULTRASONIC_HPP

#include <Arduino.h>
#include <NewPing.h>

// === 用户配置宏 ===
#define TRIG_PIN 4
#define ECHO_PIN A15
#define MAX_DISTANCE_CM 300

#ifdef __cplusplus
extern "C" {
#endif

// 初始化超声波模块
void Ultrasonic_Init(void);

// 测距（返回厘米，0 表示超时/无回波）
unsigned int Ultrasonic_GetDistance(void);

// 取中值滤波测距（更稳）
unsigned int Ultrasonic_GetDistanceMedian(uint8_t samples);

#ifdef __cplusplus
}
#endif

#endif // ULTRASONIC_HPP
