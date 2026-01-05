#ifndef _OPENMV2_H
#define _OPENMV2_H
#define SIXZEROTHREEEIGHT 0x6038
#include "ExtSRAMInterface.h"

// ==================== 全局变量声明 ====================
// 交通灯颜色存储
extern uint8_t trafficLightA_Color;      // 交通灯A颜色
extern uint8_t trafficLightB_Color;      // 交通灯B颜色
extern uint8_t trafficLightC_Color;      // 交通灯C颜色
extern uint8_t trafficLightD_Color;      // 交通灯D颜色

// 路线数据（用于其他模块）
extern uint8_t lx2[20];                  // 路线数据
extern uint8_t qr_lr;                    // 二维码左右标志
extern uint16_t QR_Num;                  // 二维码编号

// 其他模块使用的变量
extern int data1;                        // 摆正标志（用于 ali_back 等函数）
extern uint8_t light_goal;               // 光强度（用于 My_Lib.cpp）

// 兼容旧代码的变量（保留用于 SpeedSlave.ino）
extern uint8_t k1[2];                    // 卡一扇区块（兼容旧代码）
extern uint8_t k2[2];                    // 卡二扇区块（兼容旧代码）
extern uint8_t k3[2];                    // 卡三扇区块（兼容旧代码）


///新增变量//
extern char list_cp[6];

// 注意：二维码识别结果存储在 My_Lib.h 中声明的变量：
// - extern uint8_t ali_wxd[4];   // 无线电数据
// - extern uint8_t ali_fht[6];   // 烽火台数据
// - extern char ali_cp[6];       // 车牌数据
// - extern uint8_t ali_ckcs;     // 车库层数
// - extern uint8_t ali_ldC;      // 路灯初始档位
// - extern uint8_t ali_ldZ;      // 路灯最终档位

// ==================== 底层通信函数 ====================
void openMv91(uint8_t sub, uint8_t p1, uint8_t p2);
void openMv92(uint8_t sub, uint8_t p1, uint8_t p2);
void openMv94(uint8_t sub, uint8_t p1, uint8_t p2);
void emptyData(uint16_t directives);

// ==================== 舵机控制 ====================
void servoControl(int8_t angle);
void Servo_Control(int8_t angle);

// ==================== 交通灯识别 ====================
uint8_t identifyTraffic(uint8_t choice);

// ==================== 二维码识别 ====================
void Identify_QR_2(uint8_t sheet);
void Identify_colourQR_2(uint8_t sheet, uint8_t size, uint8_t around);
void rangingQR(uint8_t sheet);

// ==================== 循迹相关 ====================
void videoLine(uint8_t spend);
void videoLineMP(uint8_t spend, int distance);
void VideoLineBreak(uint8_t speed);
void Video_Line_4(uint8_t spend, uint8_t len);
void highSpeed(uint8_t spend);
void carBackTracking(uint8_t spend, int distance);

// ==================== 循迹模式控制 ====================
void OpenMVTrack_Disc_StartUp(void);
void OpenMVTrack_Disc_CloseUp(void);

// ==================== 车头摆正 ====================
int head();
void preventDie();
void headAMD();                        // 车头摆正（AMD版本）

// ==================== 倒车入库 ====================
void ali_back(char c, int isLandmark); // 倒车入库函数

// ==================== 其他功能 ====================
void initPhotosensitive(uint8_t choose);
void displayMessage(uint8_t ch, uint16_t nice);
void self_inspection();

#endif
