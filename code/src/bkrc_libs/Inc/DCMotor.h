// DCMotor.h

#ifndef _DCMOTOR_h
#define _DCMOTOR_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#define TRACK_ADDR		0x6000	//低八位循迹灯（gd）
#define TRACK_ogd		0x6001	//高八位循迹灯

#define	R_F_M_PIN		8		/*控制 右 前 边电机的引脚，输出PWM*/
#define	R_B_M_PIN		7		/*控制 右 后 边电机的引脚，输出PWM*/

#define	L_F_M_PIN		3		/*控制 左 前 边电机的引脚，输出PWM*/
#define	L_B_M_PIN		2		/*控制 左 后 边电机的引脚，输出PWM*/

#define R_CONTROL_PIN	5
#define L_CONTROL_PIN	6

#define DEFAULT_SPEED	50

#define MOVE_RUNMODE_STOP	0
#define MOVE_RUNMODE_FF		1
#define MOVE_RUNMODE_BB		2
#define MOVE_RUNMODE_LIFT	3
#define MOVE_RUNMODE_RIGHT	4
#define MOVE_RUNMODE_TRACK	5

class _DCMotor
{
public:
	_DCMotor();
	~_DCMotor();

	// 直流电机初始化：设置 PWM 频率(Hz)，并初始化引脚/定时器/占空比查表
	void Initialization(uint32_t  fHz = 80000);

	// 设置 PWM 频率（占空比保持最后一次设置值）
	void MotorFrequency(uint32_t _fHz);

	// 小车速度控制（-100~100 约定：正/反转）
	void SpeedCtr(int16_t L_speed, int16_t R_speed);

	// 小车动作
	void Stop(void);
	void StartUp(void);
	void ShutDown(void);

	// 按码盘距离转向（返回实际码盘变化）
	uint16_t TurnLeftAngle(uint8_t speed, uint16_t _distance);
	uint16_t TurnRightAngle(uint8_t speed, uint16_t _distance);

	// 行走
	void Car_Go(uint8_t speed, uint16_t distance);
	void Car_Back(uint8_t speed, uint16_t distance);

	// 码盘相关
	void Roadway_mp_sync(void);
	uint16_t Roadway_mp_Get(void);

	// 清码盘（注意：返回语义按你原逻辑：false=已清零，true=未清零需继续清）
	bool ClearCodeDisc(void);

private:
	// 占空比查表初始化
	void ParameterInit(void);

	// 单轮速度到两路 PWM 比较值映射（正转/反转）
	void SpeedSetOne(int16_t s, uint16_t* c1, uint16_t* c2);

private:
	// Timer TOP 值（OCRnA），也是内部“一个周期计数值”
	// 注意：在你的 .cpp 里 fHz 实际存的是 16000000/_fHz 的结果（更像 topCount）
	uint16_t fHz = 0;

	// 0~100% 对应的比较值（0~fHz）
	uint16_t speed[101] = {0};
};

extern _DCMotor DCMotor;

#endif
