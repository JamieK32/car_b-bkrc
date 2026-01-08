#include "ultrasonic.hpp"
#include <NewPing.h>

// 内部对象，用户不直接访问
static NewPing sonar(TRIG_PIN, ECHO_PIN, MAX_DISTANCE_CM);

void Ultrasonic_Init(void) {
  // NewPing 不需要特别初始化，只是留接口方便扩展
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
}

unsigned int Ultrasonic_GetDistance(void) {
  // 调用 NewPing 库函数，返回单位 cm
  return sonar.ping_cm();
}

unsigned int Ultrasonic_GetDistanceMedian(uint8_t samples) {
  // 多次测距取中值
  unsigned int uS = sonar.ping_median(samples);
  return sonar.convert_cm(uS);
}
